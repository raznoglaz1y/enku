
const navItems = [
  { href: "/", label: "Home", match: "/" },
  { href: "/hardware", label: "Hardware", match: "/hardware" },
  { href: "/software", label: "Software", match: "/software" },
  { href: "/design", label: "Design", match: "/design" },
  { href: "/build", label: "Build", match: "/build" },
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

const navLinks = (pathname, mobile = false) =>
  navItems
    .map(({ href, label, match }) => {
      const active = match && pathname === match ? ' class="active"' : "";
      return `<a href="${esc(href)}"${active}>${esc(label)}</a>`;
    })
    .join(mobile ? "" : "\n        ");

const headerHtml = (pathname) => `
<header class="site-header">
  <div class="header-inner shell">
    <a class="logo-link" href="/" aria-label="ENKU home">
      <img src="/assets/enku-logo.svg" alt="ENKU">
    </a>
    <nav class="desktop-nav" aria-label="Main navigation">
      ${navLinks(pathname)}
    </nav>
    <a class="github-icon" href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer" aria-label="ENKU on GitHub">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 .7a11.5 11.5 0 0 0-3.64 22.41c.58.11.79-.25.79-.56v-2.24c-3.23.7-3.91-1.37-3.91-1.37-.53-1.34-1.29-1.7-1.29-1.7-1.05-.72.08-.71.08-.71 1.17.08 1.78 1.2 1.78 1.2 1.04 1.78 2.72 1.27 3.39.97.1-.75.4-1.27.74-1.56-2.58-.29-5.29-1.29-5.29-5.68 0-1.25.45-2.28 1.2-3.08-.12-.29-.52-1.47.11-3.04 0 0 .98-.31 3.16 1.18a11 11 0 0 1 5.76 0c2.18-1.49 3.16-1.18 3.16-1.18.63 1.57.23 2.75.11 3.04.75.8 1.2 1.83 1.2 3.08 0 4.4-2.72 5.38-5.31 5.67.42.36.79 1.07.79 2.17v3.23c0 .31.21.68.8.56A11.5 11.5 0 0 0 12 .7Z"/></svg>
    </a>
    <button class="menu-button" aria-expanded="false" aria-label="Open menu"><span></span><span></span><span></span></button>
  </div>
  <nav class="mobile-nav" aria-label="Mobile navigation">
    ${navLinks(pathname, true)}
    <a href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer">GitHub ↗</a>
  </nav>
</header>`;

const sharedHeaderCss = `
<style id="enku-shared-header">
  .site-header{position:sticky!important;top:0;z-index:100;background:rgba(245,241,232,.94)!important;backdrop-filter:blur(14px);border-bottom:1px solid rgba(20,20,20,.08)}
  .site-header .header-inner{height:82px!important;display:grid!important;grid-template-columns:130px minmax(0,1fr) 36px!important;gap:22px!important;align-items:center!important}
  .site-header .logo-link{display:flex!important;align-items:center!important;justify-self:start!important}
  .site-header .logo-link img{width:124px!important;height:34px!important;object-fit:contain!important;display:block!important}
  .site-header .desktop-nav{display:flex!important;align-items:center!important;justify-content:center!important;gap:clamp(12px,1.25vw,22px)!important;font-size:13px!important;white-space:nowrap!important}
  .site-header .desktop-nav a{position:relative;opacity:.72}
  .site-header .desktop-nav a:hover,.site-header .desktop-nav a.active{opacity:1}
  .site-header .desktop-nav a.active:after{content:"";position:absolute;left:0;right:0;bottom:-12px;height:1px;background:#171717}
  .site-header .github-icon{justify-self:end!important;width:28px!important;height:28px!important;display:grid!important;place-items:center!important}
  .site-header .github-icon svg{width:24px!important;height:24px!important;fill:currentColor}
  .site-header .menu-button{display:none!important;background:transparent;border:0;padding:8px;gap:4px;flex-direction:column;justify-content:center}
  .site-header .menu-button span{display:block;width:22px;height:1px;background:#171717}
  .site-header .mobile-nav{display:none!important}
  @media(max-width:1050px){
    .site-header .header-inner{grid-template-columns:130px 1fr 36px!important}
    .site-header .desktop-nav{display:none!important}
    .site-header .github-icon{display:none!important}
    .site-header .menu-button{display:flex!important;justify-self:end}
    .site-header .mobile-nav.open{display:flex!important;flex-direction:column;gap:0;padding:6px 24px 18px;border-top:1px solid rgba(20,20,20,.08);background:#f5f1e8}
    .site-header .mobile-nav a{padding:13px 0;border-bottom:1px solid rgba(20,20,20,.08);font-size:14px}
    .site-header .mobile-nav a:last-child{border-bottom:0}
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
        .on("head", {
          element(element) {
            element.append(sharedHeaderCss, { html: true });
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
