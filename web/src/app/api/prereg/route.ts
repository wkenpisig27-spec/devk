import { NextResponse } from "next/server";
import {
  createPrereg,
  findPreregByEmail,
  findPreregByReferral,
  findPreregByUsername,
  getPreregCount,
  getPreregStats,
  trackEvent,
} from "@/lib/store";
import { factionOptions, firstJobOptions, intentOptions } from "@/data/site";

const emailRe = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
const userRe = /^[a-zA-Z][a-zA-Z0-9_]{2,15}$/;

export async function GET(req: Request) {
  const url = new URL(req.url);
  if (url.searchParams.get("stats") === "1") {
    return NextResponse.json(await getPreregStats());
  }
  return NextResponse.json({ count: await getPreregCount() });
}

export async function POST(req: Request) {
  try {
    const body = (await req.json()) as Record<string, unknown>;
    const email = String(body.email ?? "").trim();
    const username = String(body.username ?? "").trim();
    const firstJob = String(body.firstJob ?? "");
    const intent = String(body.intent ?? "");
    const faction = String(body.faction ?? "undecided");
    const referredByRaw = String(body.referredBy ?? "").trim().toUpperCase();

    if (!emailRe.test(email)) {
      return NextResponse.json({ error: "Enter a valid email." }, { status: 400 });
    }
    if (!userRe.test(username)) {
      return NextResponse.json(
        { error: "Username: 3–16 chars, start with a letter, letters/numbers/_ only." },
        { status: 400 },
      );
    }
    if (!firstJobOptions.some((j) => j.id === firstJob)) {
      return NextResponse.json({ error: "Pick a first job." }, { status: 400 });
    }
    if (!intentOptions.some((i) => i.id === intent)) {
      return NextResponse.json({ error: "Tell us if you will play at launch." }, { status: 400 });
    }
    if (!factionOptions.some((f) => f.id === faction)) {
      return NextResponse.json({ error: "Pick a faction preference." }, { status: 400 });
    }

    if (await findPreregByEmail(email)) {
      return NextResponse.json({ error: "That email is already pre-registered." }, { status: 409 });
    }
    if (await findPreregByUsername(username)) {
      return NextResponse.json({ error: "That username is already claimed." }, { status: 409 });
    }

    let referredBy: string | null = null;
    if (referredByRaw) {
      const referrer = await findPreregByReferral(referredByRaw);
      if (!referrer) {
        return NextResponse.json({ error: "Referral code not found." }, { status: 400 });
      }
      referredBy = referrer.referralCode;
    }

    const record = await createPrereg({
      email,
      username,
      firstJob,
      intent,
      faction,
      referredBy,
      userAgent: req.headers.get("user-agent") ?? undefined,
    });

    await trackEvent({
      name: "prereg_submit",
      path: "/prereg",
      meta: { firstJob, intent, faction, referred: referredBy ? "1" : "0" },
    });

    return NextResponse.json({
      ok: true,
      referralCode: record.referralCode,
      username: record.username,
      count: await getPreregCount(),
    });
  } catch (err) {
    const message = err instanceof Error ? err.message : "unknown";
    console.error("prereg_error", err);
    return NextResponse.json({ error: "Could not save pre-registration.", detail: message }, { status: 500 });
  }
}
