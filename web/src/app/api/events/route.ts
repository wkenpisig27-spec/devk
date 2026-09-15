import { NextResponse } from "next/server";
import { trackEvent } from "@/lib/store";

export async function POST(req: Request) {
  try {
    const body = (await req.json()) as {
      name?: string;
      path?: string;
      meta?: Record<string, string>;
    };
    const name = String(body.name ?? "").slice(0, 64);
    if (!name) {
      return NextResponse.json({ error: "Missing event name" }, { status: 400 });
    }
    await trackEvent({
      name,
      path: body.path ? String(body.path).slice(0, 200) : undefined,
      meta: body.meta,
    });
    return NextResponse.json({ ok: true });
  } catch {
    return NextResponse.json({ error: "track failed" }, { status: 500 });
  }
}
