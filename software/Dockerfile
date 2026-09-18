FROM python:3.12-slim

WORKDIR /app
COPY software/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r /app/requirements.txt
COPY software /app

ENV PYTHONUNBUFFERED=1
CMD ["python", "-m", "waterbox_cloud.worker"]
