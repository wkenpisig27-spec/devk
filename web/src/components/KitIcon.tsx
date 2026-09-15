export function KitIcon({ name }: { name: "fruit" | "apple" | "hp" | "sp" }) {
  const glyph = { fruit: "🍋", apple: "🍎", hp: "❤️", sp: "💧" }[name];
  return (
    <span className="text-3xl leading-none" aria-hidden>
      {glyph}
    </span>
  );
}
