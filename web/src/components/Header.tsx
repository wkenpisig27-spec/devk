"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useState } from "react";
import { nav, site } from "@/data/site";

function DiscordIcon() {
  return (
    <svg className="h-5 w-5" viewBox="0 0 24 24" fill="currentColor" aria-hidden>
      <path d="M19.27 5.33C17.94 4.71 16.5 4.26 15 4a.1.1 0 0 0-.07.03c-.18.33-.39.76-.53 1.09a16.1 16.1 0 0 0-4.8 0c-.14-.34-.37-.76-.54-1.09c-.01-.02-.04-.03-.07-.03c-1.5.26-2.93.71-4.27 1.33c-.01 0-.02.01-.03.02c-2.72 4.07-3.47 8.03-3.1 11.95c0 .02.01.04.03.05c1.8 1.32 3.53 2.12 5.24 2.65c.03.01.06 0 .07-.02c.4-.55.76-1.13 1.07-1.74c.02-.04 0-.08-.04-.1c-.57-.22-1.11-.48-1.64-.78c-.04-.02-.04-.08-.01-.11c.11-.08.22-.17.33-.25c.02-.01.05-.01.07 0c3.44 1.57 7.15 1.57 10.55 0c.02-.01.05-.01.07 0c.11.09.22.17.33.26c.04.03.04.09-.01.11c-.52.31-1.07.56-1.64.78c-.04.02-.05.07-.03.1c.32.61.68 1.19 1.07 1.74c.03.02.06.03.09.02c1.72-.53 3.45-1.33 5.25-2.65c.02-.01.03-.03.03-.05c.44-4.53-.73-8.46-3.1-11.95c-.01-.01-.02-.02-.04-.02zM8.52 14.91c-1.03 0-1.89-.95-1.89-2.12s.84-2.12 1.89-2.12c1.06 0 1.9.96 1.89 2.12c0 1.17-.84 2.12-1.89 2.12zm6.97 0c-1.03 0-1.89-.95-1.89-2.12s.84-2.12 1.89-2.12c1.06 0 1.9.96 1.89 2.12c0 1.17-.83 2.12-1.89 2.12z" />
    </svg>
  );
}

export function Header() {
  const [open, setOpen] = useState(false);
  const path = usePathname();

  return (
    <header className="sticky top-0 z-50 border-b border-line/80 bg-[#120c0c]/75 backdrop-blur-xl shadow-[0_1px_0_rgba(230,184,94,0.16)]">
      <div className="mx-auto flex max-w-[1120px] items-center gap-4 px-4 py-3 sm:px-6">
        <Link href="/" className="flex shrink-0 flex-col leading-none">
          <span className="font-display text-[1.05rem] tracking-wide text-gold drop-shadow-[0_0_12px_rgba(230,184,94,0.45)]">
            {site.shortName}
          </span>
          <span className="mt-1 text-[9px] font-semibold uppercase tracking-[0.28em] text-muted">
            Online
          </span>
        </Link>

        <nav className="ml-auto hidden items-center gap-4 lg:gap-6 md:flex" aria-label="Primary">
          {nav.map((item) => {
            const current = path === item.href;
            return (
              <Link
                key={item.href}
                href={item.href}
                aria-current={current ? "page" : undefined}
                className={`text-[15px] transition ${
                  current ? "text-gold" : "text-foam/80 hover:text-gold"
                }`}
              >
                {item.label}
              </Link>
            );
          })}
          <Link
            href="/prereg"
            aria-current={path === "/prereg" ? "page" : undefined}
            className="btn-crimson !px-3.5 !py-1.5 !text-[15px]"
          >
            Pre-register
          </Link>
          <a
            href={site.discord}
            target="_blank"
            rel="noreferrer"
            aria-label="Discord"
            className="hidden text-muted transition hover:text-gold xl:inline-flex"
          >
            <DiscordIcon />
          </a>
        </nav>

        <button
          type="button"
          className="ml-auto inline-flex h-10 w-10 items-center justify-center rounded-lg border border-line text-foam md:hidden"
          aria-label="Toggle menu"
          aria-expanded={open}
          onClick={() => setOpen((v) => !v)}
        >
          <span className="sr-only">Menu</span>
          <svg width="22" height="22" fill="none" stroke="currentColor" strokeWidth="2">
            {open ? <path d="M6 6l10 10M16 6L6 16" /> : <path d="M4 7h14M4 11h14M4 15h14" />}
          </svg>
        </button>
      </div>

      {open ? (
        <div className="border-t border-line px-4 py-4 md:hidden">
          <nav className="flex flex-col gap-3">
            {nav.map((item) => (
              <Link key={item.href} href={item.href} className="text-foam" onClick={() => setOpen(false)}>
                {item.label}
              </Link>
            ))}
            <Link href="/prereg" className="font-semibold text-gold" onClick={() => setOpen(false)}>
              Pre-register
            </Link>
            <a href={site.discord} target="_blank" rel="noreferrer" className="text-muted">
              Discord
            </a>
          </nav>
        </div>
      ) : null}
    </header>
  );
}
