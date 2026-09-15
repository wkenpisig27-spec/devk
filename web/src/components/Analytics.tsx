"use client";

import { useEffect } from "react";
import { usePathname, useSearchParams } from "next/navigation";

const REF_KEY = "abyss_ref";

export function track(name: string, meta?: Record<string, string>) {
  if (typeof window === "undefined") return;
  const payload = JSON.stringify({
    name,
    path: window.location.pathname,
    meta,
  });
  void fetch("/api/events", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: payload,
    keepalive: true,
  }).catch(() => undefined);
}

export function Analytics() {
  const pathname = usePathname();
  const search = useSearchParams();

  useEffect(() => {
    const ref = search.get("ref");
    if (ref && /^[A-Z0-9]{4,12}$/i.test(ref)) {
      try {
        localStorage.setItem(REF_KEY, ref.toUpperCase());
      } catch {
        /* ignore */
      }
    }
  }, [search]);

  useEffect(() => {
    track("page_view", { path: pathname });
  }, [pathname]);

  useEffect(() => {
    function onClick(e: MouseEvent) {
      const el = (e.target as HTMLElement | null)?.closest?.("[data-track]") as HTMLElement | null;
      if (!el) return;
      const name = el.getAttribute("data-track");
      if (!name) return;
      track(name, { href: el.getAttribute("href") ?? "" });
    }
    document.addEventListener("click", onClick);
    return () => document.removeEventListener("click", onClick);
  }, []);

  return null;
}

export function getStoredReferral(): string | null {
  if (typeof window === "undefined") return null;
  try {
    return localStorage.getItem(REF_KEY);
  } catch {
    return null;
  }
}
