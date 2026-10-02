
const primaryNav = [
  { href: "/hardware", label: "Hardware", match: "/hardware" },
  { href: "/software", label: "Software", match: "/software" },
  { href: "/design", label: "Design", match: "/design" },
  { href: "/build", label: "Build", match: "/build" },
];

const projectNav = [
  { href: "/about", label: "About", match: "/about" },
  { href: "/status", label: "Status", match: "/status" },
  { href: "/build-log", label: "Log", match: "/build-log" },
  { href: "/downloads", label: "Downloads", match: "/downloads" },
  { href: "/contact", label: "Contact", match: "/contact" },
];

const esc = (value) =>
  String(value)
    .replaceAll("&", "&amp;")
    .replaceAll('"', "&quot;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");

const activeClass = (pathname, match) =>
  match && pathname === match ? ' class="active"' : "";

const primaryLinks = (pathname) =>
  primaryNav
    .map(({ href, label, match }) =>
      `<a href="${esc(href)}"${activeClass(pathname, match)}>${esc(label)}</a>`)
    .join("");

const projectLinks = (pathname) =>
  projectNav
    .map(({ href, label, match }) =>
      `<a href="${esc(href)}"${activeClass(pathname, match)}>${esc(label)}</a>`)
    .join("");

const headerHtml = (pathname) => {
  const projectActive = projectNav.some(({ match }) => match === pathname);
  return `
<header class="site-header">
  <div class="header-inner shell">
    <a class="logo-link" href="/" aria-label="ENKU home">
      <img src="/assets/enku-logo.svg" alt="ENKU">
    </a>

    <nav class="desktop-nav" aria-label="Main navigation">
      ${primaryLinks(pathname)}
      <details class="project-menu"${projectActive ? " open" : ""}>
        <summary${projectActive ? ' class="active"' : ""}>Project <span aria-hidden="true">⌄</span></summary>
        <div class="project-popover">
          ${projectLinks(pathname)}
        </div>
      </details>
    </nav>

    <a class="github-icon" href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer" aria-label="ENKU on GitHub">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 .7a11.5 11.5 0 0 0-3.64 22.41c.58.11.79-.25.79-.56v-2.24c-3.23.7-3.91-1.37-3.91-1.37-.53-1.34-1.29-1.7-1.29-1.7-1.05-.72.08-.71.08-.71 1.17.08 1.78 1.2 1.78 1.2 1.04 1.78 2.72 1.27 3.39.97.1-.75.4-1.27.74-1.56-2.58-.29-5.29-1.29-5.29-5.68 0-1.25.45-2.28 1.2-3.08-.12-.29-.52-1.47.11-3.04 0 0 .98-.31 3.16 1.18a11 11 0 0 1 5.76 0c2.18-1.49 3.16-1.18 3.16-1.18.63 1.57.23 2.75.11 3.04.75.8 1.2 1.83 1.2 3.08 0 4.4-2.72 5.38-5.31 5.67.42.36.79 1.07.79 2.17v3.23c0 .31.21.68.8.56A11.5 11.5 0 0 0 12 .7Z"/></svg>
    </a>

    <button class="menu-button" aria-expanded="false" aria-label="Open menu"><span></span><span></span><span></span></button>
  </div>

  <nav class="mobile-nav" aria-label="Mobile navigation">
    <span class="mobile-nav-label">Explore</span>
    ${primaryLinks(pathname)}
    <span class="mobile-nav-label">Project</span>
    ${projectLinks(pathname)}
    <a href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer">GitHub ↗</a>
  </nav>
</header>`;
};

const footerHtml = `
<footer class="site-footer">
  <div class="shell enku-footer-grid">
    <div class="enku-footer-brand">
      <a href="/" aria-label="ENKU home"><img src="/assets/enku-logo.svg" alt="ENKU"></a>
      <p>Open-source pocket e-reader built in the open.</p>
    </div>
    <nav class="enku-footer-col" aria-label="Explore">
      <strong>Explore</strong>
      <a href="/hardware">Hardware</a>
      <a href="/software">Software</a>
      <a href="/design">Design</a>
      <a href="/build">Build</a>
    </nav>
    <nav class="enku-footer-col" aria-label="Project">
      <strong>Project</strong>
      <a href="/about">About</a>
      <a href="/status">Status</a>
      <a href="/build-log">Build log</a>
    </nav>
    <nav class="enku-footer-col" aria-label="Resources">
      <strong>Resources</strong>
      <a href="/downloads">Downloads</a>
      <a href="/contact">Contact</a>
      <a href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer">GitHub ↗</a>
    </nav>
  </div>
  <div class="shell enku-footer-bottom">
    <span>enkureader.com</span>
    <span>ENKU · Open source</span>
  </div>
</footer>`;

const sharedChromeCss = `
<style id="enku-shared-chrome">
  .site-header{position:sticky!important;top:0;z-index:100;background:rgba(245,241,232,.94)!important;backdrop-filter:blur(14px);border-bottom:1px solid rgba(20,20,20,.08)}
  .site-header .header-inner{height:72px!important;display:grid!important;grid-template-columns:150px minmax(0,1fr) 40px!important;gap:24px!important;align-items:center!important}
  .site-header .logo-link{display:flex!important;align-items:center!important;justify-self:start!important}
  .site-header .logo-link img{width:116px!important;height:32px!important;object-fit:contain!important;display:block!important}
  .site-header .desktop-nav{display:flex!important;align-items:center!important;justify-content:center!important;gap:clamp(18px,2vw,30px)!important;font-size:13px!important;white-space:nowrap!important}
  .site-header .desktop-nav>a,.site-header .project-menu>summary{position:relative;opacity:.68;cursor:pointer;list-style:none}
  .site-header .desktop-nav>a:hover,.site-header .desktop-nav>a.active,.site-header .project-menu>summary:hover,.site-header .project-menu>summary.active{opacity:1}
  .site-header .desktop-nav>a.active:after,.site-header .project-menu>summary.active:after{content:"";position:absolute;left:0;right:0;bottom:-10px;height:1px;background:#171717}
  .site-header .project-menu{position:relative}
  .site-header .project-menu>summary::-webkit-details-marker{display:none}
  .site-header .project-menu>summary span{display:inline-block;margin-left:4px;font-size:10px;transition:transform .18s ease}
  .site-header .project-menu[open]>summary span{transform:rotate(180deg)}
  .site-header .project-popover{position:absolute;top:30px;right:-18px;width:178px;padding:8px;background:#f5f1e8;border:1px solid rgba(20,20,20,.12);box-shadow:0 14px 36px rgba(20,20,20,.08)}
  .site-header .project-popover a{display:block;padding:10px 11px;font-size:12px;border-radius:3px}
  .site-header .project-popover a:hover,.site-header .project-popover a.active{background:rgba(20,20,20,.055)}
  .site-header .github-icon{justify-self:end!important;width:28px!important;height:28px!important;display:grid!important;place-items:center!important}
  .site-header .github-icon svg{width:23px!important;height:23px!important;fill:currentColor}
  .site-header .menu-button{display:none!important;background:transparent;border:0;padding:8px;gap:4px;flex-direction:column;justify-content:center}
  .site-header .menu-button span{display:block;width:22px;height:1px;background:#171717}
  .site-header .mobile-nav{display:none!important}
  .site-header .mobile-nav-label{font-size:9px;font-weight:700;letter-spacing:.16em;color:#817b73;padding:18px 0 5px}

  .site-footer{border-top:1px solid rgba(20,20,20,.1)!important;padding:0!important}
  .enku-footer-grid{display:grid!important;grid-template-columns:minmax(260px,1.6fr) repeat(3,minmax(120px,.7fr))!important;gap:54px!important;padding-top:58px!important;padding-bottom:50px!important;align-items:start!important}
  .enku-footer-brand img{width:116px!important;height:auto!important}
  .enku-footer-brand p{max-width:260px;margin:18px 0 0;font-size:12px;line-height:1.6;color:#6b665f}
  .enku-footer-col{display:flex!important;flex-direction:column!important;align-items:flex-start!important;gap:10px!important;font-size:12px!important;color:#55514b!important}
  .enku-footer-col strong{font-size:10px;letter-spacing:.14em;text-transform:uppercase;color:#837d75;margin-bottom:5px}
  .enku-footer-col a{opacity:.82}
  .enku-footer-col a:hover{opacity:1}
  .enku-footer-bottom{display:flex;justify-content:space-between;gap:24px;padding-top:18px!important;padding-bottom:24px!important;border-top:1px solid rgba(20,20,20,.08);font-size:10px;color:#827c74}

  @media(max-width:1050px){
    .site-header .header-inner{grid-template-columns:130px 1fr 36px!important}
    .site-header .desktop-nav{display:none!important}
    .site-header .github-icon{display:none!important}
    .site-header .menu-button{display:flex!important;justify-self:end}
    .site-header .mobile-nav.open{display:flex!important;flex-direction:column;gap:0;padding:2px 24px 18px;border-top:1px solid rgba(20,20,20,.08);background:#f5f1e8}
    .site-header .mobile-nav a{padding:11px 0;border-bottom:1px solid rgba(20,20,20,.07);font-size:14px}
    .site-header .mobile-nav a:last-child{border-bottom:0}
  }
  @media(max-width:760px){
    .site-header .header-inner{height:66px!important}
    .enku-footer-grid{grid-template-columns:1fr 1fr!important;gap:38px 28px!important;padding-top:44px!important;padding-bottom:38px!important}
    .enku-footer-brand{grid-column:1/-1}
  }
</style>`;

const allowedSubjects = new Set([
  "Project question",
  "Collaboration",
  "Hardware / firmware",
  "Design / UI",
  "Documentation",
  "Other",
]);

const json = (body, status = 200) =>
  new Response(JSON.stringify(body), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
    },
  });

const clean = (value, max) =>
  String(value ?? "")
    .replace(/\r/g, "")
    .replace(/[\u0000-\u0008\u000B\u000C\u000E-\u001F\u007F]/g, "")
    .trim()
    .slice(0, max);

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    if (url.pathname !== "/api/contact") {
      const assetResponse = await env.ASSETS.fetch(request);
      const contentType = assetResponse.headers.get("content-type") || "";
      if (!contentType.includes("text/html")) return assetResponse;

      return new HTMLRewriter()
        .on("header.site-header", {
          element(element) {
            element.replace(headerHtml(url.pathname), { html: true });
          },
        })
        .on("footer.site-footer", {
          element(element) {
            element.replace(footerHtml, { html: true });
          },
        })
        .on("a", {
          element(element) {
            const href = element.getAttribute("href");
            const routes = {
              "/#hardware": "/hardware",
              "#hardware": "/hardware",
              "/#software": "/software",
              "#software": "/software",
              "/#design": "/design",
              "#design": "/design",
              "/#build": "/build",
              "#build": "/build",
            };
            if (href && routes[href]) element.setAttribute("href", routes[href]);
          },
        })
        .on("head", {
          element(element) {
            element.append(sharedChromeCss, { html: true });
          },
        })
        .transform(assetResponse);
    }

    if (request.method !== "POST") {
      return json({ ok: false, error: "Method not allowed" }, 405);
    }

    const origin = request.headers.get("Origin");
    if (origin && origin !== url.origin) {
      return json({ ok: false, error: "Invalid origin" }, 403);
    }

    const contentType = request.headers.get("content-type") || "";
    if (!contentType.includes("application/json")) {
      return json({ ok: false, error: "Expected JSON" }, 415);
    }

    const contentLength = Number(request.headers.get("content-length") || 0);
    if (contentLength > 20000) {
      return json({ ok: false, error: "Message too large" }, 413);
    }

    let data;
    try {
      data = await request.json();
    } catch {
      return json({ ok: false, error: "Invalid request" }, 400);
    }

    // Honeypot. Real visitors never see or fill this field.
    if (clean(data.website, 200)) {
      return json({ ok: true });
    }

    const name = clean(data.name, 120);
    const email = clean(data.email, 254).toLowerCase();
    const subject = clean(data.subject, 80);
    const message = clean(data.message, 6000);
    const startedAt = Number(data.startedAt || 0);

    if (!name || !email || !subject || !message) {
      return json({ ok: false, error: "Please complete all fields." }, 400);
    }

    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
      return json({ ok: false, error: "Please enter a valid email address." }, 400);
    }

    if (!allowedSubjects.has(subject)) {
      return json({ ok: false, error: "Invalid subject." }, 400);
    }

    if (message.length < 10) {
      return json({ ok: false, error: "Please add a little more detail." }, 400);
    }

    // Basic bot-speed check. Do not reject old forms; only implausibly fast submissions.
    if (startedAt && Date.now() - startedAt < 1800) {
      return json({ ok: true });
    }

    const ip = request.headers.get("CF-Connecting-IP") || "unknown";
    const country = request.cf?.country || "unknown";
    const userAgent = clean(request.headers.get("user-agent"), 300);

    const textBody = [
      "New ENKU website message",
      "",
      `Name: ${name}`,
      `Email: ${email}`,
      `Subject: ${subject}`,
      "",
      message,
      "",
      "—",
      `IP: ${ip}`,
      `Country: ${country}`,
      `User-Agent: ${userAgent}`,
    ].join("\n");

    try {
      await env.CONTACT_EMAIL.send({
        to: "raznoglaz1y@gmail.com",
        from: { email: "contact@enkureader.com", name: "ENKU website" },
        replyTo: { email, name },
        subject: `[ENKU] ${subject} — ${name}`,
        text: textBody,
      });

      return json({ ok: true });
    } catch (error) {
      console.error("Contact email failed", error?.code, error?.message);
      return json(
        { ok: false, error: "Message could not be sent. Please try again later." },
        502,
      );
    }
  },
};
