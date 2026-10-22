// Local test server that mimics website hosting conditions (not deployed):
//  - serves the bundle under a sub-path (/learn/dsp-tutorial/) to prove all asset paths are relative
//  - sends a strict Content-Security-Policy so any inline script/style/handler would be blocked
//  - /embed-test.html loads the tutorial in an <iframe> with ?embed=1 and auto-sizes it
// Usage: node tests/serve.js [port]
"use strict";
const http = require("http");
const fs = require("fs");
const path = require("path");

const ROOT = path.join(__dirname, "..");
const BASE = "/learn/dsp-tutorial/";
const PORT = Number(process.argv[2]) || 8737;
const TYPES = { ".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8", ".png": "image/png", ".md": "text/plain; charset=utf-8" };
const CSP = "default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:; " +
  "connect-src 'self'; media-src 'self' blob:; frame-src 'self'; frame-ancestors 'self'; base-uri 'none'; form-action 'none'";

const EMBED_TEST = `<!doctype html><html><head><meta charset="utf-8"><title>Embed test</title>
<link rel="stylesheet" href="/embed-test.css"></head><body><h1>Host page</h1>
<iframe id="tut" src="${BASE}?embed=1" title="FxSound DSP Tutorial"></iframe>
<script src="/embed-test.js"></script></body></html>`;
const EMBED_JS = `window.addEventListener("message", function (e) {
  if (e.data && e.data.type === "fxsound-dsp-tutorial:height") {
    document.getElementById("tut").style.height = e.data.height + "px";
    document.title = "Embed test (height " + e.data.height + ")";
  }
});`;
const EMBED_CSS = `body{font-family:system-ui;margin:16px} iframe{width:100%;border:2px dashed #888;height:400px}`;

// Optional: --capture <dir> lets the page POST canvas PNGs (data URLs) to /__capture/<name>.png,
// used to produce the screenshots in docs/img/dsp-tutorial. Off unless the flag is given.
const captureIdx = process.argv.indexOf("--capture");
const CAPTURE_DIR = captureIdx > 0 ? path.resolve(process.argv[captureIdx + 1]) : null;

http.createServer((req, res) => {
  const url = decodeURIComponent(req.url.split("?")[0]);
  const send = (code, type, body) => { res.writeHead(code, { "Content-Type": type, "Content-Security-Policy": CSP }); res.end(body); };
  const cap = url.match(/^\/__capture\/([a-z0-9-]+\.png)$/);
  if (cap && req.method === "POST") {
    if (!CAPTURE_DIR) return send(403, "text/plain", "Capture disabled (start with --capture <dir>)");
    let body = "";
    req.on("data", (c) => { body += c; });
    req.on("end", () => {
      const b64 = body.replace(/^data:image\/png;base64,/, "");
      fs.mkdirSync(CAPTURE_DIR, { recursive: true });
      fs.writeFileSync(path.join(CAPTURE_DIR, cap[1]), Buffer.from(b64, "base64"));
      send(200, "text/plain", "saved " + cap[1]);
    });
    return;
  }
  if (url === "/embed-test.html") return send(200, TYPES[".html"], EMBED_TEST);
  if (url === "/embed-test.js") return send(200, TYPES[".js"], EMBED_JS);
  if (url === "/embed-test.css") return send(200, TYPES[".css"], EMBED_CSS);
  if (!url.startsWith(BASE)) return send(404, "text/plain", "Not found (bundle is served under " + BASE + ")");
  let file = path.normalize(path.join(ROOT, url.slice(BASE.length) || "index.html"));
  if (!file.startsWith(ROOT)) return send(403, "text/plain", "Forbidden");
  if (fs.existsSync(file) && fs.statSync(file).isDirectory()) file = path.join(file, "index.html");
  fs.readFile(file, (err, data) => err ? send(404, "text/plain", "Not found")
    : send(200, TYPES[path.extname(file)] || "application/octet-stream", data));
}).listen(PORT, () => console.log(`Serving http://localhost:${PORT}${BASE}  (embed test: http://localhost:${PORT}/embed-test.html)`));
