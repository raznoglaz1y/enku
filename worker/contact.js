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
      return json({ ok: false, error: "Not found" }, 404);
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
