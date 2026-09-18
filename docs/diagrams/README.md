# Diagram sources

HTML/CSS originals for the architecture figures (exact labels; do not regenerate with an image model).

```bash
cd docs/diagrams
for name in 01-system-flow 02-database-flow 03-mqtt-flow 04-ml-analysis-flow; do
  google-chrome-stable --headless=new --disable-gpu --no-sandbox --hide-scrollbars \
    --force-device-scale-factor=1 --window-size=1600,1000 \
    --screenshot="${name}.png" "file://$PWD/${name}.html"
done
```

Canvas is 1600×1000. Shared styles: `style.css`.
