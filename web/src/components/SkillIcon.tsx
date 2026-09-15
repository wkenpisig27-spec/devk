import type { SkillKind } from "@/data/classes";
import { skillIcons } from "@/data/skillIcons";

const stroke = { fill: "none", stroke: "currentColor", strokeWidth: 1.8, strokeLinecap: "round" as const, strokeLinejoin: "round" as const };

export function SkillIcon({ name, kind }: { name: string; kind?: SkillKind }) {
  const src = skillIcons[name];
  if (src) {
    return (
      <img
        src={src}
        alt=""
        width={32}
        height={32}
        className="h-8 w-8 shrink-0 [image-rendering:pixelated]"
      />
    );
  }

  return (
    <svg viewBox="0 0 24 24" className="h-7 w-7" aria-hidden>
      {kind === "sword" && <path {...stroke} d="M5 19l10-10M14 5l5 5M8 16l-3 3M13 8l3 3" />}
      {kind === "shield" && <path {...stroke} d="M12 3l8 3v6c0 5-3.5 8-8 9-4.5-1-8-4-8-9V6z" />}
      {kind === "rage" && <path {...stroke} d="M12 3l2 6h6l-5 4 2 7-5-4-5 4 2-7-5-4h6z" />}
      {kind === "bow" && (
        <>
          <path {...stroke} d="M5 19c8-2 12-8 14-14" />
          <path {...stroke} d="M5 19L19 5M8 12h6" />
        </>
      )}
      {kind === "gun" && <path {...stroke} d="M4 14h9l5-6h2M7 14v4M10 14v3" />}
      {kind === "heal" && <path {...stroke} d="M12 5v14M5 12h14" />}
      {kind === "bolt" && <path {...stroke} d="M13 2L4 14h7l-1 8 10-14h-7z" />}
      {kind === "poison" && <path {...stroke} d="M12 3c3 4 5 7 5 10a5 5 0 11-10 0c0-3 2-6 5-10z" />}
      {kind === "stealth" && <path {...stroke} d="M3 12s3.5-6 9-6 9 6 9 6-3.5 6-9 6-9-6-9-6zM12 12a1.5 1.5 0 100-3 1.5 1.5 0 000 3" />}
      {kind === "aoe" && (
        <>
          <circle {...stroke} cx="12" cy="12" r="3" />
          <circle {...stroke} cx="12" cy="12" r="7" />
        </>
      )}
      {kind === "speed" && <path {...stroke} d="M4 12h16M14 6l6 6-6 6" />}
      {kind === "ship" && <path {...stroke} d="M4 16l8-10 8 10H4zm0 0c2 3 12 3 16 0" />}
      {kind === "coral" && <path {...stroke} d="M12 21V9m0 0c-3-4-6-2-6-2m6 2c3-4 6-2 6-2M8 14c-3 0-4 3-4 3m12-3c3 0 4 3 4 3" />}
      {kind === "trade" && <path {...stroke} d="M4 10h6V6H4zm10 8h6v-6h-6zM10 8l4 4" />}
      {kind === "craft" && <path {...stroke} d="M14 6l4 4-8 8H6v-4zM13 7l2 2" />}
      {kind === "totem" && <path {...stroke} d="M12 21V8m-5 4h10M9 8h6l-3-5z" />}
      {kind === "seal" && (
        <>
          <circle {...stroke} cx="12" cy="12" r="7" />
          <path {...stroke} d="M9 12h6M12 9v6" />
        </>
      )}
      {kind === "buff" && <path {...stroke} d="M12 20V8m-5 4l5-5 5 5" />}
    </svg>
  );
}
