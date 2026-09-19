# Technical explanation site (GitHub Pages)

Static HTML assembled from `website/` plus `docs/diagrams`, `docs/illustrations`,
`hardware/photos` and `hardware/renders`.

Published by `.github/workflows/pages.yml` to:

**https://edwinkestler.github.io/2GWaterMeter/**

In the GitHub repo: Settings → Pages → Source = **GitHub Actions**.

## Styling

Two stylesheets, linked in that order by every page:

| File | Role |
| --- | --- |
| `css/industry.css` | The **Industry** blueprint design system — tokens (colour ramps, Barlow / Barlow Condensed, spacing, elevation) and component classes (`.blueprint` + `.corner`, `.card`, `.btn`, `.table`, `.nav`, `.tag`, `.duotone`). Source of truth for the look; retune it here. |
| `css/site.css` | The site layer only: page shell, hero, spec plate, the fingerprint instrument chart, live-state animations and the responsive rules. It never redefines a design-system token. |

House rules:

- Framed objects get `class="blueprint"` plus the four registration marks
  `<i class="corner tl">…<i class="corner br">`.
- Photographs are duotoned plates (`blueprint duotone plate`). The dark diagram
  PNGs under `docs/diagrams/` keep their own colour — frame them with
  `blueprint diagram`, never `duotone`, or their labels stop reading.
- Motion (`.wb-sweep`, `.wb-draw`, `.wb-pulse`, the nav lamp) is suppressed
  under `prefers-reduced-motion`.
- No JavaScript ships with the site; the mobile menu is a CSS-only checkbox
  toggle.

## Local preview

Assemble exactly like the workflow, then serve:

```bash
mkdir -p /tmp/wb-site/{css,diagrams,illustrations,photos,renders,docs-img}
cp website/*.html      /tmp/wb-site/
cp website/css/*       /tmp/wb-site/css/
cp docs/diagrams/*.png /tmp/wb-site/diagrams/
cp docs/illustrations/* /tmp/wb-site/illustrations/
cp hardware/photos/*   /tmp/wb-site/photos/
cp hardware/renders/*.png /tmp/wb-site/renders/
cp docs/*.png          /tmp/wb-site/docs-img/
python3 -m http.server 8000 --directory /tmp/wb-site
```
