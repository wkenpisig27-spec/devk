# Abyss-Sea Online — Website

TypeScript + Next.js (App Router) site for the landing page, progress log, and future
player portal (accounts, shop, leaderboards).

The older PHP app in `/website` still exists for live account tools. New pages should
land here so we keep one typed stack.

## Why this stack

- **TypeScript** — shared types from content today to APIs and SQL later
- **Next.js App Router** — SEO landing pages, server components, Route Handlers
- **React** — dashboards, shop, and auth UI without a second frontend
- **Runs on Node** — same Ubuntu VPS family as the game servers

## Develop

Requires Node.js 20+.

```bash
cd web
npm install
npm run dev
```

Open http://localhost:3000

## Production

```bash
cd web
npm run build
npm start
```

Default port is 3000. Put nginx in front and proxy to Node when this replaces the public
homepage.
