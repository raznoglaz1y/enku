
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
      <details class="project-menu">
        <summary${projectActive ? ' class="active"' : ""}>Project <span class="project-chevron" aria-hidden="true"></span></summary>
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

const homeHtml = `
<main class="enku-home" id="top">
  <section class="enku-home-hero">
    <div class="shell enku-home-hero-grid">
      <div class="enku-home-hero-copy">
        <p class="enku-kicker">OPEN-SOURCE E-READER</p>
        <h1>Books first.<br>Everything else second.</h1>
        <div class="enku-home-intro">
          <p>ENKU is a compact open-source e-reader built around a 3.97″ e-paper display and ESP32-S3. Hardware, software, interface and the build itself are developed as one public project.</p>
          <div class="enku-home-actions">
            <a class="enku-button" href="/build">Build ENKU <span>→</span></a>
            <a class="enku-text-link" href="https://github.com/raznoglaz1y/enku" target="_blank" rel="noreferrer">View GitHub ↗</a>
          </div>
        </div>
      </div>
      <figure class="enku-home-render">
        <img src="/assets/enku-reader-approved.webp" alt="ENKU reader design render" width="652" height="800" fetchpriority="high">
        <figcaption>Design render · physical prototype validation pending</figcaption>
      </figure>
    </div>
  </section>

  <section class="enku-home-explore">
    <div class="shell">
      <div class="enku-home-head">
        <p class="enku-kicker">EXPLORE</p>
        <h2>One reader. Four connected layers.</h2>
        <p>Each section has its own depth, but they belong to the same device. Start anywhere and continue through the project without returning to the homepage.</p>
      </div>
      <div class="enku-home-cards">
        <a class="enku-home-card" href="/hardware"><small>01 · HARDWARE</small><strong>The physical platform.</strong><p>Reference board, display, storage, controls, power and what still needs real-device validation.</p><span>Explore hardware →</span></a>
        <a class="enku-home-card" href="/software"><small>02 · SOFTWARE</small><strong>The reader core.</strong><p>Application state, parsing, pagination, rendering, persistence and the ESP-IDF device target.</p><span>Explore software →</span></a>
        <a class="enku-home-card" href="/design"><small>03 · DESIGN</small><strong>The interface system.</strong><p>E-paper-first layouts, physical navigation, typography, canonical screens and interaction rules.</p><span>Explore design →</span></a>
        <a class="enku-home-card" href="/build"><small>04 · BUILD</small><strong>From parts to reader.</strong><p>BOM, preparation, enclosure, firmware, assembly and first boot—with prototype-dependent steps marked clearly.</p><span>Explore build →</span></a>
      </div>
    </div>
  </section>

  <section class="enku-home-status">
    <div class="shell enku-home-split">
      <div>
        <p class="enku-kicker">CURRENT STATUS</p>
        <h2>Firmware first.<br>Hardware next.</h2>
      </div>
      <div>
        <p>The reader architecture, host-tested behavior and ESP-IDF platform work are already in the repository. The decisive next step is physical validation on the Waveshare reference hardware, followed by the enclosure prototype.</p>
        <div class="enku-home-status-grid">
          <div><small>NOW</small><strong>Firmware + bring-up</strong></div>
          <div><small>NEXT</small><strong>Physical validation</strong></div>
          <div><small>THEN</small><strong>Mechanical prototype</strong></div>
        </div>
        <a class="enku-text-link" href="/status">Full project status →</a>
      </div>
    </div>
  </section>

  <section class="enku-home-work">
    <div class="shell">
      <div class="enku-home-head compact">
        <p class="enku-kicker">LATEST WORK</p>
        <h2>Follow the engineering record.</h2>
      </div>
      <div class="enku-home-work-grid">
        <a href="/build-log" class="enku-work-main"><small>BUILD LOG</small><strong>From repository to reader.</strong><p>See the chronological record of firmware, UI, hardware and website milestones—with the failures and corrections kept visible.</p><span>Open build log →</span></a>
        <div class="enku-work-side">
          <a href="/downloads"><small>RELEASES</small><strong>Downloads</strong><span>No fake release files. Only reproducible artifacts when they are ready. →</span></a>
          <a href="/contact"><small>OPEN PROJECT</small><strong>Contribute</strong><span>Questions, testing, code, documentation, hardware knowledge or support. →</span></a>
        </div>
      </div>
    </div>
  </section>

  <section class="enku-home-final">
    <a class="shell enku-home-final-inner" href="/about">
      <span><small>ABOUT ENKU</small><strong>Why this reader exists.</strong></span><b>→</b>
    </a>
  </section>
</main>`;

const nextPage = (pathname) => {
  const flow = {
    "/hardware": { href: "/software", eyebrow: "NEXT · SOFTWARE", title: "See how the reader works." },
    "/software": { href: "/design", eyebrow: "NEXT · DESIGN", title: "See the interface system." },
    "/design": { href: "/build", eyebrow: "NEXT · BUILD", title: "Turn the system into a device." },
    "/build": { href: "/status", eyebrow: "NEXT · STATUS", title: "Follow what is verified now." },
    "/about": { href: "/status", eyebrow: "NEXT · STATUS", title: "See where the project stands." },
    "/status": { href: "/build-log", eyebrow: "NEXT · BUILD LOG", title: "Read the engineering record." },
    "/build-log": { href: "/downloads", eyebrow: "NEXT · DOWNLOADS", title: "Find reproducible releases." },
    "/downloads": { href: "/contact", eyebrow: "NEXT · CONTACT", title: "Get involved with ENKU." },
    "/contact": { href: "/hardware", eyebrow: "EXPLORE · HARDWARE", title: "Start from the reference platform." },
  };
  const item = flow[pathname];
  if (!item) return "";
  return `
<section class="enku-next">
  <a class="shell enku-next-inner" href="${item.href}">
    <span>
      <small>${item.eyebrow}</small>
      <strong>${item.title}</strong>
    </span>
    <b aria-hidden="true">→</b>
  </a>
</section>`;
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
  .site-header .desktop-nav>a,.site-header .project-menu>summary{position:relative;opacity:.68;cursor:pointer;list-style:none;padding:7px 8px;border-radius:5px;transition:opacity .16s ease,background .16s ease}
  .site-header .desktop-nav>a:hover,.site-header .project-menu>summary:hover{opacity:1;background:rgba(20,20,20,.04)}
  .site-header .desktop-nav>a.active,.site-header .project-menu>summary.active{opacity:1;font-weight:600;background:rgba(20,20,20,.055)}
  .site-header .project-menu{position:relative}
  .site-header .project-menu>summary::-webkit-details-marker{display:none}
  .site-header .project-chevron{display:inline-block!important;width:7px!important;height:7px!important;margin-left:7px!important;border-right:1px solid currentColor;border-bottom:1px solid currentColor;transform:translateY(-2px) rotate(45deg);transition:transform .18s ease}
  .site-header .project-menu[open] .project-chevron{transform:translateY(2px) rotate(225deg)}
  .site-header .project-popover{position:absolute;top:30px;right:-18px;width:178px;padding:8px;background:#f5f1e8;border:1px solid rgba(20,20,20,.12);box-shadow:0 14px 36px rgba(20,20,20,.08)}
  .site-header .project-popover a{display:block;padding:10px 11px;font-size:12px;border-radius:3px}
  .site-header .project-popover a:hover,.site-header .project-popover a.active{background:rgba(20,20,20,.055)}
  .site-header .github-icon{justify-self:end!important;width:28px!important;height:28px!important;display:grid!important;place-items:center!important}
  .site-header .github-icon svg{width:23px!important;height:23px!important;fill:currentColor}
  .site-header .menu-button{display:none!important;background:transparent;border:0;padding:8px;gap:4px;flex-direction:column;justify-content:center}
  .site-header .menu-button span{display:block;width:22px;height:1px;background:#171717}
  .site-header .mobile-nav{display:none!important}
  .site-header .mobile-nav-label{font-size:9px;font-weight:700;letter-spacing:.16em;color:#817b73;padding:18px 0 5px}

  .enku-home{background:#f5f1e8}
  .enku-home-hero{padding:104px 0 96px;border-bottom:1px solid rgba(20,20,20,.09)}
  .enku-home-hero-grid{display:grid;grid-template-columns:minmax(0,1.12fr) minmax(300px,.72fr);gap:72px;align-items:center}
  .enku-home-split{display:grid;grid-template-columns:.9fr 1.1fr;gap:96px;align-items:start}
  .enku-kicker{margin:0 0 17px;font-size:10px;font-weight:700;letter-spacing:.17em;color:#777168}
  .enku-home h1,.enku-home h2,.enku-home-card strong,.enku-work-main strong,.enku-work-side strong,.enku-home-final strong{font-family:"Newsreader",serif;font-weight:500;letter-spacing:-.04em}
  .enku-home h1{font-size:clamp(68px,7.1vw,108px);line-height:.87;margin:0}
  .enku-home-intro{margin-top:38px}
  .enku-home-intro>p{font-size:17px;line-height:1.68;color:#5d5851;max-width:610px;margin:0}
  .enku-home-actions{display:flex;align-items:center;gap:22px;margin-top:30px;flex-wrap:wrap}
  .enku-home-render{margin:0;justify-self:end;width:min(100%,430px);text-align:center}
  .enku-home-render img{display:block;width:100%;height:auto;object-fit:contain}
  .enku-home-render figcaption{margin-top:13px;font-size:9px;line-height:1.4;letter-spacing:.08em;text-transform:uppercase;color:#8a847b}
  .enku-button{min-height:50px;padding:0 20px;border:1px solid #171717;border-radius:5px;background:#171717;color:#f5f1e8;display:inline-flex;align-items:center;gap:26px;font-size:13px;font-weight:600}
  .enku-text-link{font-size:13px;font-weight:600;border-bottom:1px solid rgba(20,20,20,.25);padding-bottom:3px}
  .enku-home-explore,.enku-home-status,.enku-home-work{padding:108px 0;border-bottom:1px solid rgba(20,20,20,.09)}
  .enku-home-head{display:grid;grid-template-columns:.42fr .92fr 1fr;gap:56px;align-items:start;margin-bottom:62px}
  .enku-home-head h2,.enku-home-split h2{font-size:clamp(46px,4.8vw,68px);line-height:.94;margin:0}
  .enku-home-head>p:last-child,.enku-home-split>div:last-child>p{font-size:15px;line-height:1.7;color:#625e57;margin:0}
  .enku-home-cards{display:grid;grid-template-columns:repeat(4,1fr);border-top:1px solid rgba(20,20,20,.16);border-bottom:1px solid rgba(20,20,20,.16);column-gap:0}
  .enku-home-card{padding:30px 26px 30px 0;min-height:328px;display:flex;flex-direction:column;transition:transform .18s ease,background .18s ease}
  .enku-home-card+.enku-home-card{border-left:1px solid rgba(20,20,20,.12);padding-left:26px}
  .enku-home-card small,.enku-work-main small,.enku-work-side small,.enku-home-final small,.enku-home-status-grid small{font-size:9px;font-weight:700;letter-spacing:.13em;color:#7c766e}
  .enku-home-card strong{font-size:32px;line-height:1.04;margin:58px 0 13px}
  .enku-home-card p{font-size:12px;line-height:1.68;color:#666159;margin:0;max-width:245px}
  .enku-home-card span{margin-top:auto;padding-top:30px;font-size:12px;font-weight:600}.enku-home-card:hover{transform:translateY(-3px);background:rgba(255,255,255,.14)}
  .enku-home-split{align-items:start}
  .enku-home-status-grid{display:grid;grid-template-columns:repeat(3,1fr);border-top:1px solid rgba(20,20,20,.15);border-bottom:1px solid rgba(20,20,20,.15);margin:38px 0 30px}
  .enku-home-status-grid div{padding:20px 18px 22px 0}
  .enku-home-status-grid div+div{border-left:1px solid rgba(20,20,20,.12);padding-left:18px}
  .enku-home-status-grid strong{display:block;font-family:"Newsreader",serif;font-size:23px;font-weight:500;margin-top:7px}
  .enku-home-head.compact{grid-template-columns:.4fr 1.6fr;margin-bottom:42px}
  .enku-home-work-grid{display:grid;grid-template-columns:1.25fr .75fr;gap:28px}
  .enku-work-main,.enku-work-side a{border:1px solid rgba(20,20,20,.13);padding:32px;display:flex;flex-direction:column;transition:transform .18s ease,background .18s ease}
  .enku-work-main{min-height:330px}
  .enku-work-main strong{font-size:42px;line-height:1;margin:54px 0 12px}
  .enku-work-main p{font-size:13px;line-height:1.65;color:#645f58;max-width:580px;margin:0}
  .enku-work-main span,.enku-work-side span{margin-top:auto;font-size:12px;font-weight:600;padding-top:28px}.enku-work-main:hover,.enku-work-side a:hover{transform:translateY(-3px);background:rgba(255,255,255,.14)}
  .enku-work-side{display:grid;grid-template-rows:1fr 1fr;gap:18px}
  .enku-work-side strong{font-size:28px;margin:20px 0 8px}
  .enku-work-side span{font-weight:400;line-height:1.5;color:#625e57}
  .enku-home-final{border-bottom:1px solid rgba(20,20,20,.09)}
  .enku-home-final-inner{min-height:164px;display:flex;align-items:center;justify-content:space-between;gap:30px}
  .enku-home-final strong{display:block;font-size:42px;margin-top:7px}
  .enku-home-final b{font-size:34px;font-weight:400}
  .enku-next{border-top:1px solid rgba(20,20,20,.1);background:rgba(255,255,255,.13)}
  .enku-next-inner{min-height:132px;display:flex;align-items:center;justify-content:space-between;gap:32px}
  .enku-next small{display:block;font-size:9px;font-weight:700;letter-spacing:.16em;color:#837d75;margin-bottom:8px}
  .enku-next strong{font-family:"Newsreader",serif;font-size:clamp(28px,3vw,42px);font-weight:500;letter-spacing:-.025em}
  .enku-next b{font-size:32px;font-weight:400;transition:transform .18s ease}
  .enku-next-inner:hover b{transform:translateX(5px)}
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
  @media(max-width:900px){
    .enku-home-hero-grid,.enku-home-split{grid-template-columns:1fr;gap:42px}
    .enku-home-render{justify-self:center;width:min(72vw,380px)}
    .enku-home-head{grid-template-columns:1fr;gap:14px}
    .enku-home-cards{grid-template-columns:1fr 1fr}
    .enku-home-card:nth-child(3){border-left:0;border-top:1px solid rgba(20,20,20,.12);padding-left:0}
    .enku-home-card:nth-child(4){border-top:1px solid rgba(20,20,20,.12)}
    .enku-home-work-grid{grid-template-columns:1fr}
  }
  @media(max-width:760px){
    .site-header .header-inner{height:66px!important}
    .enku-footer-grid{grid-template-columns:1fr 1fr!important;gap:38px 28px!important;padding-top:44px!important;padding-bottom:38px!important}
    .enku-footer-brand{grid-column:1/-1}
    .enku-home-hero{padding:70px 0 70px}
    .enku-home h1{font-size:clamp(58px,16vw,82px)}
    .enku-home-intro{margin-top:30px}
    .enku-home-render{width:min(78vw,330px)}
    .enku-home-explore,.enku-home-status,.enku-home-work{padding:78px 0}
    .enku-home-cards{grid-template-columns:1fr}
    .enku-home-card,.enku-home-card+.enku-home-card,.enku-home-card:nth-child(3),.enku-home-card:nth-child(4){border-left:0;border-top:1px solid rgba(20,20,20,.12);padding:24px 0;min-height:240px}
    .enku-home-card:first-child{border-top:0}
    .enku-home-card strong{margin-top:30px}
    .enku-home-status-grid{grid-template-columns:1fr}
    .enku-home-status-grid div,.enku-home-status-grid div+div{border-left:0;border-top:1px solid rgba(20,20,20,.1);padding:16px 0}
    .enku-home-status-grid div:first-child{border-top:0}
    .enku-home-final strong{font-size:32px}
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
        .on("main", {
          element(element) {
            if (url.pathname === "/") element.replace(homeHtml, { html: true });
          },
        })
        .on("footer.site-footer", {
          element(element) {
            const next = nextPage(url.pathname);
            if (next) element.before(next, { html: true });
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
