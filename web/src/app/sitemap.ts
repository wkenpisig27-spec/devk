import type { MetadataRoute } from "next";
import { site } from "@/data/site";

export default function sitemap(): MetadataRoute.Sitemap {
  const routes = ["", "/features", "/progress", "/classes", "/prereg", "/guides/pko-classes"];
  const now = new Date();
  return routes.map((route) => ({
    url: `${site.url}${route}`,
    lastModified: now,
    changeFrequency: route === "" || route === "/progress" ? "weekly" : "monthly",
    priority: route === "" ? 1 : route === "/prereg" || route === "/classes" ? 0.9 : 0.7,
  }));
}
