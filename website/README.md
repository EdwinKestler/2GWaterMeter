# Technical explanation site (GitHub Pages)

Static HTML assembled from `website/` plus `docs/diagrams`, `docs/illustrations`, and `hardware/photos`.

Published by `.github/workflows/pages.yml` to:

**https://edwinkestler.github.io/2GWaterMeter/**

In the GitHub repo: Settings → Pages → Source = **GitHub Actions**.

Local preview after a copy:

```bash
mkdir -p /tmp/wb-site && cp website/*.html /tmp/wb-site/
# open website/index.html via a static server at repo root after assembling like the workflow
```
