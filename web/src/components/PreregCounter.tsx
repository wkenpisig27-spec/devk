"use client";

import { useEffect, useState } from "react";

export function PreregCounter({ className = "" }: { className?: string }) {
  const [count, setCount] = useState<number | null>(null);

  useEffect(() => {
    let cancelled = false;
    void fetch("/api/prereg")
      .then((r) => r.json())
      .then((d: { count?: number }) => {
        if (!cancelled && typeof d.count === "number") setCount(d.count);
      })
      .catch(() => undefined);
    return () => {
      cancelled = true;
    };
  }, []);

  if (count === null) {
    return (
      <p className={`text-sm text-muted ${className}`}>
        Sailors are claiming names…
      </p>
    );
  }

  return (
    <p className={`text-sm text-gold ${className}`}>
      <span className="font-semibold">{count.toLocaleString()}</span>
      {count === 1 ? " sailor has" : " sailors have"} claimed a spot
    </p>
  );
}
