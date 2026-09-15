import type { Metadata } from "next";
import { ClassExplorer } from "@/components/ClassExplorer";
import { Shell } from "@/components/Shell";

export const metadata: Metadata = {
  title: "Classes",
  description:
    "Browse Abyss-Sea Online classes and skills — Swordsman, Hunter, Herbalist, Explorer, and every advancement.",
};

export default function ClassesPage() {
  return (
    <Shell>
      <div className="mx-auto max-w-[1120px] px-4 py-16 sm:px-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">
          Four first jobs. Then a class quest.
        </p>
        <h1 className="title-glow font-display mt-3 max-w-[40rem] text-[2.6rem] leading-none tracking-wide sm:text-[3.4rem]">
          Class tree
        </h1>
        <p className="mt-5 max-w-[40rem] text-[17px] leading-7 text-muted">
          Swordsman, Hunter, Herbalist, Explorer. Pick one to read its skills and the second
          jobs you can quest into — Crusader, Champion, Sharpshooter, Cleric, Seal Master, Voyager.
        </p>
        <ClassExplorer />
      </div>
    </Shell>
  );
}
