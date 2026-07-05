#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <vector>
#include "../App.h"

// HTML Frontend embedded in PROGMEM
const char webui_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>5herbet PDA | Web Manager</title>
    <script src="https://cdn.jsdelivr.net/npm/marked/marked.min.js"></script>
    <style>
        :root {
            --bg: #1a1a1e;
            --panel: #25252b;
            --accent: #ff477e;
            --accent-dim: #992849;
            --text-main: #f0f0f5;
            --text-dim: #9e9ea6;
            --border: #3b3b45;
            --success: #20d98f;
        }
        * { box-sizing: border-box; }
        body { 
            margin: 0; padding: 0; font-family: 'Inter', -apple-system, sans-serif; 
            background: var(--bg); color: var(--text-main); 
            height: 100vh; display: flex; flex-direction: column;
        }
        header {
            background: var(--panel); border-bottom: 1px solid var(--border);
            padding: 1rem 2rem; display: flex; justify-content: space-between; align-items: center;
        }
        h1 { margin: 0; font-size: 1.2rem; color: var(--accent); }
        .tabs { display: flex; gap: 1rem; }
        .tab { 
            padding: 0.5rem 1rem; cursor: pointer; border-radius: 4px; 
            color: var(--text-dim); transition: all 0.2s;
        }
        .tab:hover { background: rgba(255,255,255,0.05); }
        .tab.active { background: var(--accent); color: white; }
        
        main { flex: 1; padding: 2rem; max-width: 1400px; margin: 0 auto; width: 100%; overflow-y: auto; }
        .view { display: none; height: 100%; flex-direction: column; }
        .view.active { display: flex; animation: fadeIn 0.3s; }
        
        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

        /* File Explorer */
        .toolbar { display: flex; gap: 1rem; margin-bottom: 1rem; align-items: center; flex-shrink: 0; }
        .btn { 
            background: var(--panel); border: 1px solid var(--border); color: var(--text-main);
            padding: 0.5rem 1rem; border-radius: 4px; cursor: pointer; transition: 0.2s;
        }
        .btn:hover { border-color: var(--accent); color: var(--accent); }
        .btn-primary { background: var(--accent); border-color: var(--accent); color: white; }
        .btn-primary:hover { background: var(--accent-dim); color: white; }
        .btn-danger { color: #ff4444; }

        .path-display { font-family: monospace; color: var(--accent); margin-right: auto; }
        
        table { width: 100%; border-collapse: collapse; background: var(--panel); border-radius: 8px; overflow: hidden; }
        th, td { padding: 1rem; text-align: left; border-bottom: 1px solid var(--border); }
        th { color: var(--text-dim); font-weight: 500; }
        tr:hover { background: rgba(255,255,255,0.02); }
        .col-actions { text-align: right; }
        
        /* Editor */
        .split-pane { display: flex; gap: 1rem; flex: 1; min-height: 0; overflow: hidden; }
        .pane { flex: 1; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; overflow: auto; }
        #editor { width: 100%; height: 100%; background: transparent; color: var(--text-main);
            border: none; padding: 1rem; font-family: monospace; font-size: 14px; resize: none; outline: none; }
        #preview { padding: 1.5rem; line-height: 1.6; }
        
        /* Markdown Preview Styles */
        #preview h1, #preview h2, #preview h3 { color: var(--accent); margin-top: 0; }
        #preview code { background: rgba(0,0,0,0.3); padding: 0.2rem 0.4rem; border-radius: 4px; font-family: monospace; }
        #preview pre { background: rgba(0,0,0,0.3); padding: 1rem; border-radius: 8px; overflow-x: auto; }
        #preview pre code { background: transparent; padding: 0; }
        #preview blockquote { border-left: 4px solid var(--accent); margin: 0; padding-left: 1rem; color: var(--text-dim); }
        #preview a { color: var(--accent); text-decoration: none; }
        #preview a:hover { text-decoration: underline; }
            
        /* Organizer */
        .org-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 2rem; flex: 1; min-height: 0; }
        .card { background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 1.5rem; display: flex; flex-direction: column; overflow: hidden; }
        .card h2 { margin-top: 0; font-size: 1.1rem; color: var(--accent); border-bottom: 1px solid var(--border); padding-bottom: 0.5rem; display: flex; justify-content: space-between; align-items: center; }
        
        .item-list { flex: 1; overflow-y: auto; padding-right: 0.5rem; }
        .item-row { display: flex; align-items: center; gap: 0.5rem; padding: 0.75rem 0; border-bottom: 1px solid rgba(255,255,255,0.05); }
        .item-row:last-child { border-bottom: none; }
        .item-row.done { opacity: 0.5; text-decoration: line-through; }
        .item-row input[type="text"] { flex: 1; background: transparent; border: none; color: var(--text-main); font-size: 1rem; outline: none; }
        .item-row input[type="date"] { background: rgba(0,0,0,0.2); border: 1px solid var(--border); color: var(--text-dim); padding: 0.2rem; border-radius: 4px; outline: none; color-scheme: dark; }
        .item-row input[type="checkbox"] { width: 1.2rem; height: 1.2rem; cursor: pointer; accent-color: var(--accent); }

        /* Modal */
        .modal-overlay { display: none; position: fixed; top:0; left:0; width:100%; height:100%; background: rgba(0,0,0,0.7); z-index: 1000; align-items: center; justify-content: center; }
        .modal-overlay.active { display: flex; animation: fadeIn 0.2s; }
        .modal { background: var(--panel); border: 1px solid var(--border); border-radius: 8px; width: 500px; max-width: 90%; max-height: 80vh; display: flex; flex-direction: column; padding: 1.5rem; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        .modal h2 { margin-top: 0; color: var(--accent); }
        .modal-list-container { flex: 1; overflow-y: auto; border: 1px solid var(--border); border-radius: 4px; background: rgba(0,0,0,0.2); margin-top: 1rem; margin-bottom: 1rem; min-height: 200px; }
        .modal-actions { display: flex; justify-content: flex-end; gap: 1rem; }
    </style>
</head>
<body>
    <header>
        <h1>5herbet PDA</h1>
        <div class="tabs">
            <div class="tab active" onclick="switchView('files')">Files</div>
            <div class="tab" onclick="switchView('editor')">Editor</div>
            <div class="tab" onclick="switchView('organizer')">Organizer</div>
        </div>
    </header>

    <main>
        <!-- FILE EXPLORER -->
        <div id="view-files" class="view active">
            <div class="toolbar">
                <span class="path-display" id="current-path">/</span>
                <input type="file" id="upload-input" style="display:none" onchange="uploadFile()">
                <button class="btn btn-primary" onclick="document.getElementById('upload-input').click()">Upload File</button>
                <button class="btn" onclick="newFolder()">New Folder</button>
            </div>
            <table>
                <thead><tr><th>Name</th><th>Size</th><th class="col-actions">Actions</th></tr></thead>
                <tbody id="file-list">
                    <tr><td colspan="3" style="text-align:center">Loading...</td></tr>
                </tbody>
            </table>
        </div>

        <!-- EDITOR -->
        <div id="view-editor" class="view">
            <div class="toolbar">
                <input type="text" id="edit-path" class="btn" style="flex: 1; text-align:left; cursor:text;" placeholder="File path (e.g. /5herbetPDA/Notes/idea.md)">
                <button class="btn" onclick="loadFileForEdit()">Load</button>
                <button class="btn btn-primary" onclick="saveFile()">Save</button>
            </div>
            <div class="split-pane">
                <div class="pane"><textarea id="editor" placeholder="Write markdown here..." oninput="updatePreview()"></textarea></div>
                <div class="pane" id="preview"></div>
            </div>
        </div>

        <!-- ORGANIZER -->
        <div id="view-organizer" class="view">
            <div class="toolbar">
                <button class="btn btn-primary" onclick="loadOrganizer()">Load / Refresh</button>
                <button class="btn btn-primary" style="background:var(--success);border-color:var(--success);" onclick="saveOrganizer()">Save Changes to PDA</button>
            </div>
            <div class="org-grid">
                <div class="card">
                    <h2>Tasks <button class="btn" style="padding: 0.2rem 0.5rem;" onclick="addTask()">+ Add</button></h2>
                    <div id="task-list" class="item-list"></div>
                </div>
                <div class="card">
                    <h2>Events <button class="btn" style="padding: 0.2rem 0.5rem;" onclick="addEvent()">+ Add</button></h2>
                    <div id="event-list" class="item-list"></div>
                </div>
            </div>
        </div>

        <!-- DESTINATION PICKER MODAL -->
        <div id="modal-overlay" class="modal-overlay">
            <div class="modal">
                <h2 id="modal-title">Select Destination</h2>
                <div class="toolbar" style="margin-top: 1rem; margin-bottom: 0;">
                    <span class="path-display" id="modal-path">/</span>
                </div>
                <div class="modal-list-container">
                    <table style="margin:0; border-radius:0;">
                        <tbody id="modal-file-list"></tbody>
                    </table>
                </div>
                <div class="modal-actions">
                    <button class="btn" onclick="closeModal()">Cancel</button>
                    <button class="btn btn-primary" id="modal-confirm-btn" onclick="confirmModalAction()">Select</button>
                </div>
            </div>
        </div>
    </main>

    <script>
        let currentPath = "/";
        let organizerData = { tasks: [], events: [] };

        function switchView(viewName) {
            document.querySelectorAll('.view').forEach(el => el.classList.remove('active'));
            document.querySelectorAll('.tab').forEach(el => el.classList.remove('active'));
            document.getElementById('view-' + viewName).classList.add('active');
            event.target.classList.add('active');
            
            if (viewName === 'files') loadFiles(currentPath);
            if (viewName === 'organizer') loadOrganizer();
        }

        // --- FILES ---
        async function loadFiles(dir) {
            const res = await fetch(`/api/files?dir=${encodeURIComponent(dir)}`);
            const data = await res.json();
            currentPath = dir;
            document.getElementById('current-path').textContent = dir;
            
            let html = '';
            if (dir !== "/") {
                const parent = dir.split('/').slice(0, -1).join('/') || '/';
                html += `<tr><td><a href="#" onclick="loadFiles('${parent}')" style="color:inherit;text-decoration:none">📁 ..</a></td><td>-</td><td></td></tr>`;
            }
            
            data.forEach(f => {
                const isDir = f.isDir;
                const path = dir === "/" ? `/${f.name}` : `${dir}/${f.name}`;
                const icon = isDir ? '📁' : '📄';
                const nameLink = isDir 
                    ? `<a href="#" onclick="loadFiles('${path}')" style="color:inherit;text-decoration:none">${icon} ${f.name}</a>`
                    : `<span style="cursor:pointer" onclick="openInEditor('${path}')">${icon} ${f.name}</span>`;
                
                const size = isDir ? '-' : (f.size < 1024 ? f.size + ' B' : (f.size/1024).toFixed(1) + ' KB');
                
                html += `<tr>
                    <td>${nameLink}</td>
                    <td>${size}</td>
                    <td class="col-actions">
                        <button class="btn" style="padding:0.2rem 0.5rem" title="Rename" onclick="renameItem('${path}')">✏️</button>
                        <button class="btn" style="padding:0.2rem 0.5rem" title="Move" onclick="moveItem('${path}')">✂️</button>
                        <button class="btn" style="padding:0.2rem 0.5rem" title="Copy" onclick="copyItem('${path}', ${isDir})">📋</button>
                        ${!isDir ? `<button class="btn" style="padding:0.2rem 0.5rem" title="Download" onclick="downloadFile('${path}')">⬇️</button>` : ''}
                        <button class="btn btn-danger" style="padding:0.2rem 0.5rem" title="Delete" onclick="deleteFile('${path}', ${isDir})">✖</button>
                    </td>
                </tr>`;
            });
            document.getElementById('file-list').innerHTML = html;
        }

        async function uploadFile() {
            const file = document.getElementById('upload-input').files[0];
            if (!file) return;
            const formData = new FormData();
            formData.append("file", file, file.name);
            await fetch(`/api/upload?dir=${encodeURIComponent(currentPath)}`, { method: 'POST', body: formData });
            loadFiles(currentPath);
        }

        async function newFolder() {
            const name = prompt("Folder name:");
            if (!name) return;
            await fetch(`/api/folder?path=${encodeURIComponent(currentPath === '/' ? '/' + name : currentPath + '/' + name)}`, { method: 'POST' });
            loadFiles(currentPath);
        }

        async function deleteFile(path, isDir) {
            if (!confirm(`Delete ${path}?`)) return;
            const res = await fetch(`/api/file?path=${encodeURIComponent(path)}&isDir=${isDir}`, { method: 'DELETE' });
            if (!res.ok) alert("Failed to delete " + path);
            loadFiles(currentPath);
        }

        function downloadFile(path) {
            window.location.href = `/api/file?path=${encodeURIComponent(path)}`;
        }

        async function renameItem(oldPath) {
            const newName = prompt(`Rename ${oldPath} to:`, oldPath.split('/').pop());
            if (!newName) return;
            const dir = oldPath.split('/').slice(0, -1).join('/') || '/';
            const newPath = dir === '/' ? `/${newName}` : `${dir}/${newName}`;
            if (oldPath === newPath) return;
            await fetch(`/api/rename?oldPath=${encodeURIComponent(oldPath)}&newPath=${encodeURIComponent(newPath)}`, { method: 'POST' });
            loadFiles(currentPath);
        }

        // --- MODAL ---
        let modalState = { active: false, action: null, targetPath: null, isDir: false, currentPath: '/' };

        function openModal(action, targetPath, isDir) {
            modalState = { active: true, action, targetPath, isDir, currentPath: '/' };
            const title = action === 'move' ? `Move ${targetPath.split('/').pop()}` : `Copy ${targetPath.split('/').pop()}`;
            document.getElementById('modal-title').textContent = title;
            document.getElementById('modal-confirm-btn').textContent = action === 'move' ? 'Move Here' : 'Copy Here';
            document.getElementById('modal-overlay').classList.add('active');
            loadModalFiles('/');
        }

        function closeModal() {
            document.getElementById('modal-overlay').classList.remove('active');
            modalState.active = false;
        }

        async function loadModalFiles(dir) {
            const res = await fetch(`/api/files?dir=${encodeURIComponent(dir)}`);
            const data = await res.json();
            modalState.currentPath = dir;
            document.getElementById('modal-path').textContent = dir;
            
            let html = '';
            if (dir !== "/") {
                const parent = dir.split('/').slice(0, -1).join('/') || '/';
                html += `<tr><td style="padding: 0.5rem;"><a href="#" onclick="loadModalFiles('${parent}')" style="color:inherit;text-decoration:none">📁 ..</a></td></tr>`;
            }
            
            data.forEach(f => {
                if (f.isDir) {
                    const path = dir === "/" ? `/${f.name}` : `${dir}/${f.name}`;
                    html += `<tr><td style="padding: 0.5rem;"><a href="#" onclick="loadModalFiles('${path}')" style="color:inherit;text-decoration:none">📁 ${f.name}</a></td></tr>`;
                }
            });
            if (html === '') html = `<tr><td style="padding: 0.5rem; color: var(--text-dim); text-align: center;">Empty Folder</td></tr>`;
            document.getElementById('modal-file-list').innerHTML = html;
        }

        async function confirmModalAction() {
            const filename = modalState.targetPath.split('/').pop();
            const destName = modalState.action === 'copy' ? filename.replace(/(\.[\w\d_-]+)$/i, '_copy$1') : filename;
            const finalName = modalState.action === 'copy' && destName === filename ? filename + "_copy" : destName;
            
            const newPath = modalState.currentPath === '/' ? `/${finalName}` : `${modalState.currentPath}/${finalName}`;
            
            if (modalState.targetPath === newPath) {
                alert("Source and destination are the same.");
                closeModal();
                return;
            }

            const endpoint = modalState.action === 'move' ? '/api/rename' : '/api/copy';
            const res = await fetch(`${endpoint}?oldPath=${encodeURIComponent(modalState.targetPath)}&newPath=${encodeURIComponent(newPath)}`, { method: 'POST' });
            
            if (!res.ok) alert(`${modalState.action === 'move' ? 'Move' : 'Copy'} failed!`);
            
            closeModal();
            loadFiles(currentPath);
        }

        function moveItem(oldPath) {
            openModal('move', oldPath, false);
        }

        function copyItem(oldPath, isDir) {
            if (isDir) { alert("Copying directories is not supported yet."); return; }
            openModal('copy', oldPath, isDir);
        }

        // --- EDITOR ---
        function openInEditor(path) {
            document.getElementById('edit-path').value = path;
            switchView('editor');
            document.querySelectorAll('.tab')[1].classList.add('active');
            document.querySelectorAll('.tab')[0].classList.remove('active');
            loadFileForEdit();
        }

        async function loadFileForEdit() {
            const path = document.getElementById('edit-path').value;
            if (!path) return;
            const res = await fetch(`/api/file?path=${encodeURIComponent(path)}`);
            if (res.ok) {
                document.getElementById('editor').value = await res.text();
            } else {
                document.getElementById('editor').value = "";
            }
            updatePreview();
        }

        async function saveFile() {
            const path = document.getElementById('edit-path').value;
            const content = document.getElementById('editor').value;
            if (!path) return;
            const res = await fetch(`/api/file?path=${encodeURIComponent(path)}`, {
                method: 'POST',
                body: content
            });
            if (res.ok) alert("Saved!");
            else alert("Save failed!");
        }

        function updatePreview() {
            const text = document.getElementById('editor').value;
            document.getElementById('preview').innerHTML = marked.parse(text);
        }

        // --- ORGANIZER ---
        async function loadOrganizer() {
            const res = await fetch(`/api/organizer`);
            if (!res.ok) return;
            organizerData = await res.json();
            if (!organizerData.tasks) organizerData.tasks = [];
            if (!organizerData.events) organizerData.events = [];
            renderOrganizer();
        }

        function renderOrganizer() {
            const tList = document.getElementById('task-list');
            tList.innerHTML = organizerData.tasks.map((t, i) => `
                <div class="item-row ${t.completed ? 'done' : ''}">
                    <input type="checkbox" ${t.completed ? 'checked' : ''} onchange="updateTask(${i}, 'completed', this.checked)">
                    <input type="text" value="${t.title}" onchange="updateTask(${i}, 'title', this.value)">
                    <button class="btn btn-danger" style="padding:0.2rem 0.5rem;" onclick="delTask(${i})">✖</button>
                </div>
            `).join('');

            const eList = document.getElementById('event-list');
            eList.innerHTML = organizerData.events.map((e, i) => `
                <div class="item-row">
                    <input type="date" value="${e.date}" onchange="updateEvent(${i}, 'date', this.value)">
                    <input type="text" value="${e.title}" onchange="updateEvent(${i}, 'title', this.value)">
                    <button class="btn btn-danger" style="padding:0.2rem 0.5rem;" onclick="delEvent(${i})">✖</button>
                </div>
            `).join('');
        }

        function addTask() {
            organizerData.tasks.push({ title: "New Task", completed: false, level: 0, priority: 0, deadline: "", tags: [] });
            renderOrganizer();
        }

        function delTask(i) { organizerData.tasks.splice(i, 1); renderOrganizer(); }

        function updateTask(i, field, val) { 
            organizerData.tasks[i][field] = val; 
            if(field === 'completed') renderOrganizer();
        }

        function addEvent() {
            const today = new Date().toISOString().split('T')[0];
            organizerData.events.push({ title: "New Event", date: today, type: "EVENT" });
            renderOrganizer();
        }

        function delEvent(i) { organizerData.events.splice(i, 1); renderOrganizer(); }
        function updateEvent(i, field, val) { organizerData.events[i][field] = val; }

        async function saveOrganizer() {
            const res = await fetch(`/api/organizer`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(organizerData)
            });
            if (res.ok) alert("Organizer updated on PDA!");
            else alert("Failed to update organizer.");
        }

        // Init
        loadFiles("/");
    </script>
</body>
</html>
)rawliteral";


class WebUIServerApp {
private:
    WebServer* server = nullptr;
    bool isRunning = false;

    void handleFilesList() {
        if (!SD.begin()) {
            server->send(500, "application/json", "{\"error\":\"SD Card not found\"}");
            return;
        }

        String dirPath = server->arg("dir");
        if (dirPath == "") dirPath = "/";

        File dir = SD.open(dirPath);
        if (!dir || !dir.isDirectory()) {
            server->send(404, "application/json", "{\"error\":\"Directory not found\"}");
            return;
        }

        String json = "[";
        File file = dir.openNextFile();
        bool first = true;
        while (file) {
            if (!first) json += ",";
            String fname = String(file.name());
            
            // Remove full path from filename if present (ESP32 SD library sometimes returns full path)
            int lastSlash = fname.lastIndexOf('/');
            if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);

            json += "{\"name\":\"" + fname + "\",";
            json += "\"isDir\":" + String(file.isDirectory() ? "true" : "false") + ",";
            json += "\"size\":" + String(file.size()) + "}";
            first = false;
            file = dir.openNextFile();
        }
        json += "]";
        server->send(200, "application/json", json);
    }

    void handleFileDownload() {
        String path = server->arg("path");
        if (!SD.exists(path)) {
            server->send(404, "text/plain", "File not found");
            return;
        }
        File f = SD.open(path, FILE_READ);
        if (!f) {
            server->send(500, "text/plain", "Failed to open file");
            return;
        }
        
        String dataType = "application/octet-stream";
        if (path.endsWith(".txt") || path.endsWith(".md")) dataType = "text/plain";
        else if (path.endsWith(".json")) dataType = "application/json";

        server->streamFile(f, dataType);
        f.close();
    }

    void handleFileUpload() {
        // Handled in two parts: handling the POST headers, and handling the file data
        server->send(200, "text/plain", "File uploaded");
    }
    
    // For large uploads, WebServer uses onFileUpload
    void handleUploadData() {
        HTTPUpload& upload = server->upload();
        String dir = server->arg("dir");
        if (dir == "") dir = "/";
        if (dir == "/") dir = "";

        static File uploadFile;
        if (upload.status == UPLOAD_FILE_START) {
            String filename = upload.filename;
            if (!filename.startsWith("/")) filename = "/" + filename;
            String fullPath = dir + filename;
            if (SD.exists(fullPath)) SD.remove(fullPath);
            uploadFile = SD.open(fullPath, FILE_WRITE);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile) {
                uploadFile.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile) {
                uploadFile.close();
            }
        }
    }

    void handleFileWrite() {
        String path = server->arg("path");
        if (path == "") {
            server->send(400, "text/plain", "Path required");
            return;
        }
        File f = SD.open(path, FILE_WRITE);
        if (!f) {
            server->send(500, "text/plain", "Failed to open file for writing");
            return;
        }
        f.print(server->arg("plain"));
        f.close();
        server->send(200, "text/plain", "Saved");
    }

    bool deleteRecursive(String path) {
        File f = SD.open(path);
        if (!f) return false;
        if (!f.isDirectory()) {
            f.close();
            return SD.remove(path);
        }
        std::vector<String> children;
        File child = f.openNextFile();
        while (child) {
            String childPath = String(child.path());
            if (!childPath.startsWith("/")) {
                childPath = path;
                if (!childPath.endsWith("/")) childPath += "/";
                childPath += child.name();
            }
            children.push_back(childPath);
            child.close();
            child = f.openNextFile();
        }
        f.close();
        for (const String& childPath : children) {
            if (!deleteRecursive(childPath)) return false;
        }
        return SD.rmdir(path);
    }

    void handleFileDelete() {
        String path = server->arg("path");
        if (!SD.exists(path)) {
            server->send(404, "text/plain", "Not found");
            return;
        }
        bool success = deleteRecursive(path);
        if (success) server->send(200, "text/plain", "Deleted");
        else server->send(500, "text/plain", "Delete failed");
    }

    void handleMkdir() {
        String path = server->arg("path");
        if (SD.mkdir(path)) server->send(200, "text/plain", "Created");
        else server->send(500, "text/plain", "Failed");
    }

    void handleOrganizerGet() {
        if (!SD.exists("/5herbetPDA/organizer/data.json")) {
            server->send(404, "application/json", "{}");
            return;
        }
        File f = SD.open("/5herbetPDA/organizer/data.json", FILE_READ);
        server->streamFile(f, "application/json");
        f.close();
    }

    void handleOrganizerPost() {
        if (!SD.exists("/5herbetPDA/organizer")) SD.mkdir("/5herbetPDA/organizer");
        File f = SD.open("/5herbetPDA/organizer/data.json", FILE_WRITE);
        if (!f) {
            server->send(500, "text/plain", "Failed to open file for writing");
            return;
        }
        f.print(server->arg("plain"));
        f.close();
        server->send(200, "text/plain", "Saved");
    }

    void handleRename() {
        String oldPath = server->arg("oldPath");
        String newPath = server->arg("newPath");
        if (oldPath == "" || newPath == "") {
            server->send(400, "text/plain", "Missing paths");
            return;
        }
        if (SD.rename(oldPath, newPath)) {
            server->send(200, "text/plain", "Renamed");
        } else {
            server->send(500, "text/plain", "Failed to rename/move");
        }
    }

    void handleCopy() {
        String oldPath = server->arg("oldPath");
        String newPath = server->arg("newPath");
        if (oldPath == "" || newPath == "") {
            server->send(400, "text/plain", "Missing paths");
            return;
        }
        File src = SD.open(oldPath, FILE_READ);
        if (!src || src.isDirectory()) {
            server->send(400, "text/plain", "Source not a file");
            if (src) src.close();
            return;
        }
        if (SD.exists(newPath)) SD.remove(newPath);
        File dst = SD.open(newPath, FILE_WRITE);
        if (!dst) {
            server->send(500, "text/plain", "Failed to open dest");
            src.close();
            return;
        }
        
        uint8_t buf[1024];
        while (src.available()) {
            int bytesRead = src.read(buf, sizeof(buf));
            dst.write(buf, bytesRead);
        }
        src.close();
        dst.close();
        server->send(200, "text/plain", "Copied");
    }

public:
    void start() {
        if (isRunning) return;
        server = new WebServer(80);

        server->on("/", HTTP_GET, [this]() {
            server->send_P(200, "text/html", webui_html);
        });

        server->on("/api/files", HTTP_GET, std::bind(&WebUIServerApp::handleFilesList, this));
        server->on("/api/file", HTTP_GET, std::bind(&WebUIServerApp::handleFileDownload, this));
        server->on("/api/file", HTTP_POST, std::bind(&WebUIServerApp::handleFileWrite, this));
        server->on("/api/file", HTTP_DELETE, std::bind(&WebUIServerApp::handleFileDelete, this));
        server->on("/api/folder", HTTP_POST, std::bind(&WebUIServerApp::handleMkdir, this));
        
        server->on("/api/upload", HTTP_POST, 
            [this](){ server->send(200, "text/plain", ""); },
            std::bind(&WebUIServerApp::handleUploadData, this)
        );

        server->on("/api/organizer", HTTP_GET, std::bind(&WebUIServerApp::handleOrganizerGet, this));
        server->on("/api/organizer", HTTP_POST, std::bind(&WebUIServerApp::handleOrganizerPost, this));
        server->on("/api/rename", HTTP_POST, std::bind(&WebUIServerApp::handleRename, this));
        server->on("/api/copy", HTTP_POST, std::bind(&WebUIServerApp::handleCopy, this));

        server->begin();
        isRunning = true;
    }

    void stop() {
        if (!isRunning) return;
        server->stop();
        delete server;
        server = nullptr;
        isRunning = false;
    }

    void handleClient() {
        if (isRunning && server) {
            server->handleClient();
        }
    }

    bool active() const { return isRunning; }
};
