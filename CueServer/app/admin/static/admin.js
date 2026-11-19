const $ = (id) => document.getElementById(id);
const form = $("cue-form");
const cueList = $("cue-list");
const showSelect = $("show-select");
const showDialog = $("show-dialog");
const decision = $("decision");
const defaults = {
  name: "", animationState: "Idle", cameraState: "Full", lightingState: "Neutral",
  appearancePreset: "Preset_01",
};
const labels = {
  name: "이름", animationState: "Animation", cameraState: "Camera",
  lightingState: "Lighting", appearancePreset: "Appearance",
};
const symbols = { saved: "✓", error: "!", dirty: "✎", busy: "◌" };

let shows = [];
let current = null;
let selected = null;
let draft = null;
let dirty = false;
let busy = false;
let reloadTarget = null;
let showEditId = null;
let draggedRow = null;
let dropMarker = null;

function status(message, state = "saved") {
  const icon = document.createElement("span");
  icon.setAttribute("aria-hidden", "true");
  icon.textContent = symbols[state];
  $("status").dataset.state = state;
  $("status").replaceChildren(icon, ` ${message}`);
}

async function api(path = "", method = "GET", data) {
  const options = { method, cache: "no-store" };
  if (data !== undefined) {
    options.headers = { "Content-Type": "application/json" };
    options.body = JSON.stringify(data);
  }
  let response;
  try {
    response = await fetch(`/api/v1/shows${path}`, options);
  } catch {
    throw new Error("서버 연결을 확인하고 목록을 새로고침하세요.");
  }
  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    throw Object.assign(new Error(error.message ?? `요청 실패 (${response.status})`), {
      fieldErrors: error.fieldErrors ?? [],
    });
  }
  return response.status === 204 ? null : response.json();
}

function formatNumber(number) {
  return String(number).padStart(3, "0");
}

function updateControls() {
  const blocked = busy || reloadTarget !== null;
  const index = current?.cues.findIndex((cue) => cue.id === selected?.id) ?? -1;
  showSelect.disabled = blocked || shows.length === 0;
  $("new-show").disabled = blocked;
  $("reload").disabled = busy;
  for (const id of ["rename-show", "delete-show", "new-cue"]) {
    $(id).disabled = blocked || !current;
  }
  $("move-up").disabled = blocked || index < 1;
  $("move-down").disabled = blocked || index < 0 || index >= current.cues.length - 1;
  $("cue-fields").disabled = blocked;
  $("save-cue").disabled = !dirty;
  $("discard-cue").disabled = !dirty;
  $("duplicate-cue").disabled = !selected;
  $("delete-cue").disabled = !selected;
  $("save-show").disabled = busy;
  $("cancel-show").disabled = busy;
  $("show-name").disabled = busy;
  $("download").hidden = !current || blocked;
  for (const row of cueList.querySelectorAll("button")) row.disabled = blocked;
  for (const handle of cueList.querySelectorAll(".cue-drag-handle")) {
    handle.draggable = !blocked && current.cues.length > 1;
  }
}

function renderList() {
  finishDrag();
  const cues = current?.cues ?? [];
  $("count").textContent = cues.length;
  $("empty").textContent = current ? "등록된 Cue가 없습니다." : "Show를 만들거나 선택하세요.";
  $("empty").hidden = cues.length > 0;
  cueList.replaceChildren(...cues.map((cue) => {
    const li = document.createElement("li");
    li.dataset.dropCueId = cue.id;
    const handle = document.createElement("span");
    handle.className = "cue-drag-handle";
    handle.textContent = "⠿";
    handle.title = "드래그하여 Cue 순서 변경";
    handle.setAttribute("aria-hidden", "true");
    const row = document.createElement("button");
    row.type = "button";
    row.className = "cue-row";
    row.dataset.cueId = cue.id;
    row.setAttribute("aria-pressed", String(cue.id === selected?.id));
    row.onclick = async () => {
      if (cue.id === selected?.id || !(await allowLeave())) return;
      openEditor(current.cues.find((entry) => entry.id === cue.id) ?? null);
      focusCue(cue.id);
      status("저장된 Cue를 표시 중입니다.");
    };
    const number = document.createElement("span");
    number.className = "cue-number";
    number.textContent = formatNumber(cue.number);
    const content = document.createElement("span");
    const name = document.createElement("span");
    name.className = "cue-name";
    name.textContent = cue.name;
    const states = document.createElement("span");
    states.className = "cue-states";
    states.textContent = [cue.animationState, cue.cameraState, cue.lightingState, cue.appearancePreset].join(" · ");
    content.append(name, states);
    row.append(number, content);
    if (cue.id === selected?.id) {
      const selection = document.createElement("span");
      selection.className = "selection";
      selection.textContent = "선택됨";
      selection.setAttribute("aria-hidden", "true");
      row.append(selection);
    }
    li.append(handle, row);
    return li;
  }));
}

function focusCue(id) {
  cueList.querySelector(`[data-cue-id="${id}"]`)?.focus();
}

function clearDropMarker() {
  dropMarker?.classList.remove("drop-before", "drop-after");
  dropMarker = null;
}

function finishDrag() {
  clearDropMarker();
  draggedRow?.classList.remove("dragging");
  draggedRow = null;
}

function dropTarget(event) {
  const row = event.target.closest("li[data-drop-cue-id]")
    ?? Array.from(cueList.children).find((item) => event.clientY < item.getBoundingClientRect().bottom)
    ?? cueList.lastElementChild;
  const bounds = row.getBoundingClientRect();
  return { row, after: event.clientY >= bounds.top + bounds.height / 2 };
}

cueList.addEventListener("dragstart", (event) => {
  if (!event.target.matches(".cue-drag-handle[draggable=true]") || busy || reloadTarget) {
    event.preventDefault();
    return;
  }
  draggedRow = event.target.parentElement;
  event.dataTransfer.effectAllowed = "move";
  event.dataTransfer.setData("application/x-cue-id", draggedRow.dataset.dropCueId);
  draggedRow.classList.add("dragging");
});

cueList.addEventListener("dragover", (event) => {
  if (!draggedRow || busy || reloadTarget) return;
  event.preventDefault();
  event.dataTransfer.dropEffect = "move";
  const { row, after } = dropTarget(event);
  if (dropMarker !== row) clearDropMarker();
  dropMarker = row;
  row.classList.toggle("drop-before", !after);
  row.classList.toggle("drop-after", after);
});

cueList.addEventListener("dragleave", (event) => {
  if (!cueList.contains(event.relatedTarget)) clearDropMarker();
});

cueList.addEventListener("dragend", finishDrag);
cueList.addEventListener("drop", (event) => {
  if (!draggedRow || busy || reloadTarget) return;
  event.preventDefault();
  const { row, after } = dropTarget(event);
  const cueId = draggedRow.dataset.dropCueId;
  finishDrag();
  reorderCue(cueId, row.dataset.dropCueId, after);
});

function openEditor(cue, newDraft = null) {
  selected = cue;
  draft = newDraft;
  dirty = draft !== null;
  const values = selected ?? draft?.values;
  form.hidden = !values;
  $("inspector-empty").hidden = Boolean(values);
  if (values) {
    for (const field of Object.keys(defaults)) form.elements.namedItem(field).value = values[field];
    $("cue-heading").textContent = selected ? `Cue ${formatNumber(selected.number)} · ${selected.name}` : draft.title;
    $("discard-cue").textContent = draft ? "초안 취소" : "변경 취소";
  }
  renderCueErrors();
  renderList();
  updateControls();
}

function renderCueErrors(error) {
  const summary = $("cue-errors");
  summary.replaceChildren();
  summary.hidden = !error;
  for (const field of Object.keys(defaults)) {
    $(`error-${field}`).hidden = true;
    $(`cue-${field}`).removeAttribute("aria-invalid");
  }
  if (!error) return;

  summary.textContent = error.message;
  const details = document.createElement("ul");
  let firstInvalid = null;
  for (const { field, message } of error.fieldErrors ?? []) {
    const item = document.createElement("li");
    if (Object.hasOwn(labels, field)) {
      const input = $(`cue-${field}`);
      const note = $(`error-${field}`);
      const link = document.createElement("a");
      link.href = `#${input.id}`;
      link.textContent = `${labels[field]} · ${message}`;
      link.onclick = (event) => { event.preventDefault(); input.focus(); };
      note.textContent = message;
      note.hidden = false;
      input.setAttribute("aria-invalid", "true");
      item.append(link);
      firstInvalid ??= input;
    } else {
      item.textContent = message;
    }
    details.append(item);
  }
  if (details.children.length) summary.append(details);
  firstInvalid?.focus();
}

async function load(showId = current?.id, cueId = selected?.id) {
  const list = await api();
  const target = list.find((show) => show.id === showId) ?? list[0];
  const show = target ? await api(`/${target.id}`) : null;
  shows = list;
  current = show;
  reloadTarget = null;
  showSelect.replaceChildren(...shows.map((entry) => new Option(entry.name, entry.id)));
  if (show) {
    showSelect.value = show.id;
    $("revision").textContent = `Revision ${show.revision}`;
    $("download").href = `/api/v1/shows/${show.id}/cue-list`;
    $("download").download = `${show.id}-r${show.revision}.json`;
  } else {
    showSelect.append(new Option("등록된 Show 없음", ""));
    $("revision").textContent = "Revision —";
  }
  openEditor(show?.cues.find((cue) => cue.id === cueId) ?? show?.cues[0] ?? null);
}

async function refresh(showId, cueId) {
  busy = true;
  updateControls();
  status("목록을 불러오는 중…", "busy");
  try {
    await load(showId, cueId);
    status(current ? "저장된 목록을 불러왔습니다." : "새 Show를 만들어 시작하세요.");
    return true;
  } catch (error) {
    status(`조회 실패 · ${error.message}`, "error");
    return false;
  } finally {
    busy = false;
    updateControls();
  }
}

async function saveAndReload(operation, target, success, showError) {
  busy = true;
  updateControls();
  status("저장 중…", "busy");
  let failure;
  try {
    const result = await operation();
    reloadTarget = target(result);
    dirty = false;
    await load(...reloadTarget);
    status(success);
    return true;
  } catch (error) {
    failure = error;
    status(reloadTarget ? "저장 완료 · 목록 조회 실패. 새로고침을 눌러 주세요." : `저장 실패 · ${error.message}`, "error");
    return false;
  } finally {
    busy = false;
    updateControls();
    if (failure && !reloadTarget) showError?.(failure);
  }
}

function ask(title, message, deleting = false) {
  $("decision-title").textContent = title;
  $("decision-message").textContent = message;
  $("discard-choice").hidden = deleting;
  $("save-choice").hidden = deleting;
  $("delete-choice").hidden = !deleting;
  return new Promise((resolve) => {
    const finish = (event, choice) => {
      event.preventDefault();
      decision.close();
      resolve(choice);
    };
    decision.querySelector("form").onsubmit = (event) => finish(event, event.submitter?.value ?? "cancel");
    decision.oncancel = (event) => finish(event, "cancel");
    decision.showModal();
    $("keep-editing").focus();
  });
}

async function allowLeave() {
  if (!dirty) return true;
  const answer = await ask("저장하지 않은 변경", "Cue 변경을 저장할까요?");
  if (answer === "save") return saveCue();
  if (answer !== "discard") return false;
  discardChanges();
  return true;
}

function discardChanges() {
  const previous = selected ?? current.cues.find((cue) => cue.id === draft?.previousId) ?? null;
  openEditor(previous);
  status("저장된 내용으로 돌아왔습니다.");
}

function openDraft(source) {
  const values = Object.fromEntries(Object.keys(defaults).map((field) => [field, (source ?? defaults)[field]]));
  // Preserve stored transition data when cloning, without exposing transition controls.
  const preservedTimes = source ? { cameraBlendMs: source.cameraBlendMs, lightingFadeMs: source.lightingFadeMs } : {};
  openEditor(null, { title: source ? `Cue ${formatNumber(source.number)}의 복제 초안` : "새 Cue 초안", values, preservedTimes, previousId: selected?.id });
  status("저장 전 변경 · 초안", "dirty");
  $("cue-name").focus();
  $("cue-name").select();
}

function cuePayload() {
  const data = Object.fromEntries(new FormData(form));
  const fieldErrors = [];
  data.name = data.name.trim();
  if (!data.name || [...data.name].length > 100) {
    fieldErrors.push({ field: "name", message: "이름을 1~100자로 입력하세요." });
  }
  if (fieldErrors.length) throw Object.assign(new Error("입력값을 확인하세요."), { fieldErrors });
  return { ...data, ...draft?.preservedTimes };
}

async function saveCue() {
  if (busy || !dirty || reloadTarget) return false;
  renderCueErrors();
  let data;
  try {
    data = cuePayload();
  } catch (error) {
    status("저장 실패 · 입력값을 확인하세요.", "error");
    renderCueErrors(error);
    return false;
  }
  const showId = current.id;
  const cueId = selected?.id;
  const saved = await saveAndReload(
    () => api(`/${showId}/cues${cueId ? `/${cueId}` : ""}`, cueId ? "PATCH" : "POST", data),
    (cue) => [showId, cue.id], "저장 완료 · Cue를 저장했습니다.", renderCueErrors,
  );
  if (saved && selected) focusCue(selected.id);
  return saved;
}

function insertionOrder(cueId, anchorId, after) {
  if (cueId === anchorId) return null;
  const ids = current.cues.map((cue) => cue.id);
  const source = ids.indexOf(cueId);
  const anchor = ids.indexOf(anchorId);
  if (source < 0 || anchor < 0) return null;
  const destination = anchor + Number(after) - Number(source < anchor);
  if (source === destination) return null;
  ids.splice(source, 1);
  ids.splice(destination, 0, cueId);
  return ids;
}

async function reorderCue(cueId, anchorId, after, moveButton) {
  if (busy || reloadTarget || !insertionOrder(cueId, anchorId, after)) return;
  const showId = current.id;
  if (!(await allowLeave()) || current?.id !== showId) return;
  const cueIds = insertionOrder(cueId, anchorId, after);
  if (!cueIds) return;
  const selectedId = selected?.id;
  // Render the new order only after the API confirms the write.
  await saveAndReload(
    () => api(`/${showId}/cue-order`, "PUT", { cueIds }),
    () => [showId, selectedId], "저장 완료 · Cue 순서를 변경했습니다.",
  );
  if (moveButton && !moveButton.disabled) moveButton.focus();
  else focusCue(cueId);
}

function moveCue(offset) {
  const index = current.cues.findIndex((cue) => cue.id === selected?.id);
  const anchor = current.cues[index + offset];
  if (index >= 0 && anchor) reorderCue(selected.id, anchor.id, offset > 0, $(offset < 0 ? "move-up" : "move-down"));
}

function showNameError(message = "") {
  $("show-error").textContent = message;
  $("show-error").hidden = !message;
  if (message) {
    $("show-name").setAttribute("aria-invalid", "true");
    $("show-name").focus();
  } else {
    $("show-name").removeAttribute("aria-invalid");
  }
}

async function openShowForm(edit) {
  if (!(await allowLeave())) return;
  showEditId = edit ? current.id : null;
  $("show-dialog-title").textContent = edit ? "Show 이름 수정" : "새 Show";
  $("show-name").value = edit ? current.name : "";
  showNameError();
  showDialog.showModal();
  $("show-name").select();
}

$("new-show").onclick = () => openShowForm(false);
$("rename-show").onclick = () => openShowForm(true);
$("cancel-show").onclick = () => showDialog.close();
showDialog.addEventListener("cancel", (event) => { if (busy) event.preventDefault(); });
$("show-form").onsubmit = async (event) => {
  event.preventDefault();
  if (busy) return;
  const name = $("show-name").value.trim();
  if (!name || [...name].length > 100) {
    showNameError("이름을 1~100자로 입력하세요.");
    return;
  }
  showNameError();
  const cueId = showEditId ? selected?.id : undefined;
  const saved = await saveAndReload(
    () => api(showEditId ? `/${showEditId}` : "", showEditId ? "PATCH" : "POST", { name }),
    (show) => [show.id, cueId], "저장 완료 · Show를 저장했습니다.",
    (error) => showNameError(error.fieldErrors?.[0]?.message ?? error.message),
  );
  if (saved || reloadTarget) showDialog.close();
  if (saved) showSelect.focus();
};

showSelect.onchange = async () => {
  const id = showSelect.value;
  showSelect.value = current?.id ?? "";
  if (!(await allowLeave())) return;
  await refresh(id, null);
  showSelect.focus();
};
$("reload").onclick = async () => {
  if (await allowLeave()) await refresh(...(reloadTarget ?? []));
  $("reload").focus();
};
$("delete-show").onclick = async () => {
  const { id, name } = current;
  if (await ask("Show 삭제", `“${name}” Show와 소속 Cue를 삭제할까요?`, true) !== "delete") return;
  await saveAndReload(() => api(`/${id}`, "DELETE"), () => [id, null], "저장 완료 · Show를 삭제했습니다.");
  $("new-show").focus();
};

$("new-cue").onclick = async () => { if (await allowLeave()) openDraft(null); };
$("duplicate-cue").onclick = async () => { if (await allowLeave()) openDraft(selected); };
$("discard-cue").onclick = () => {
  discardChanges();
  if (selected) focusCue(selected.id);
  else $("new-cue").focus();
};
$("delete-cue").onclick = async () => {
  const showId = current.id;
  const { id, number, name } = selected;
  if (await ask("Cue 삭제", `Cue ${formatNumber(number)} “${name}”을 삭제할까요?`, true) !== "delete") return;
  const saved = await saveAndReload(() => api(`/${showId}/cues/${id}`, "DELETE"), () => [showId, null], "저장 완료 · Cue를 삭제했습니다.");
  if (saved && selected) focusCue(selected.id);
  else if (saved) $("new-cue").focus();
};
$("move-up").onclick = () => moveCue(-1);
$("move-down").onclick = () => moveCue(1);
form.oninput = () => {
  dirty = true;
  updateControls();
  status("저장 전 변경", "dirty");
};
form.onsubmit = (event) => { event.preventDefault(); saveCue(); };
window.addEventListener("beforeunload", (event) => {
  if (dirty) { event.preventDefault(); event.returnValue = ""; }
});

refresh();
