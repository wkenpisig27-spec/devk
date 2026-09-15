import Link from "next/link";
import { site } from "@/data/site";

export function Footer() {
  return (
    <footer className="mt-20 border-t border-line bg-ink-deep">
      <div className="mx-auto flex max-w-[1120px] flex-col gap-6 px-4 py-10 sm:px-6 md:flex-row md:items-start md:justify-between">
        <div>
          <p className="font-display text-lg text-gold drop-shadow-[0_0_12px_rgba(230,184,94,0.35)]">{site.name}</p>
          <p className="mt-2 max-w-sm text-sm leading-relaxed text-muted">
            A fan-run Pirates King Online world. Pick a flag, find a crew, and make the
            harbors remember you.
          </p>
        </div>
        <ul className="flex flex-wrap gap-x-6 gap-y-2 text-sm text-foam/80">
          <li>
            <Link href="/prereg" className="hover:text-gold">
              Pre-register
            </Link>
          </li>
          <li>
            <Link href="/features" className="hover:text-gold">
              Features
            </Link>
          </li>
          <li>
            <Link href="/progress" className="hover:text-gold">
              News
            </Link>
          </li>
          <li>
            <Link href="/classes" className="hover:text-gold">
              Classes
            </Link>
          </li>
          <li>
            <a href={site.discord} target="_blank" rel="noreferrer" className="hover:text-gold">
              Discord
            </a>
          </li>
        </ul>
      </div>
      <div className="border-t border-line py-4 text-center text-xs text-muted">
        © {new Date().getFullYear()} Abyss-Sea community. Fan-run, non-commercial. Pirates
        King Online and related marks belong to their original owners.
      </div>
    </footer>
  );
}
