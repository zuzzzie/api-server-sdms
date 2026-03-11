# Deployment Guide — IoT API Server on Render

## Overview
This guide deploys the smart dustbin API server on [Render](https://render.com) (free tier). Note that Render's free tier provides HTTPS automatically — your ESP32 device sends plain HTTP, which Render handles by upgrading to HTTPS on the domain side.

---

## Step 1: Push to GitHub

Make sure your project is committed and pushed to a GitHub repository:

```bash
git init
git add .
git commit -m "Initial IoT API server"
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

---

## Step 2: Create a Web Service on Render

1. Go to [https://render.com](https://render.com) and sign in.
2. Click **New +** → **Web Service**.
3. Connect your GitHub account and select your repository.
4. Fill in the settings:
   - **Name**: `api-server-sdms` (or any name you prefer)
   - **Region**: Choose closest to your device's SIM network
   - **Branch**: `main`
   - **Build Command**: `npm install`
   - **Start Command**: `node server.js`
   - **Plan**: Free

5. Click **Create Web Service**.

---

## Step 3: Set Environment Variables

In the Render dashboard for your service, go to **Environment** tab and add:

| Key | Value |
|-----|-------|
| `SUPABASE_URL` | Your Supabase project URL (e.g. `https://abc123.supabase.co`) |
| `SUPABASE_SERVICE_ROLE_KEY` | Your Supabase service role key (from Project Settings → API) |
| `API_KEY` | A strong secret string that your ESP32 device will send in `x-api-key` header |

> **Do NOT set `PORT`** — Render injects this automatically.

---

## Step 4: Verify Health Check

Once deployed, Render will show your public URL (e.g. `https://api-server-sdms.onrender.com`).

Test the health endpoint:

```bash
curl https://api-server-sdms.onrender.com/health
```

Expected response:
```json
{ "status": "ok", "uptime": 12 }
```

If Render's health checks are configured, set the **Health Check Path** to `/health` in the service settings.

---

## Step 5: Test the Full Endpoint (Local or Live)

**Local test:**
```bash
curl -X POST http://localhost:3000/update-location \
  -H "Content-Type: application/json" \
  -H "x-api-key: your_secret_key_for_devices" \
  -d '{"bin_id": "BIN_01", "lat": 13.0827, "lng": 80.2707}'
```

**Live test on Render:**
```bash
curl -X POST https://api-server-sdms.onrender.com/update-location \
  -H "Content-Type: application/json" \
  -H "x-api-key: your_secret_key_for_devices" \
  -d '{"bin_id": "BIN_01", "lat": 13.0827, "lng": 80.2707}'
```

Expected response:
```json
{ "status": "success" }
```

---

## HTTP vs HTTPS Note

- **Render free tier provides HTTPS** on the `.onrender.com` domain automatically.
- **Your ESP32 / SIM800L** sends raw HTTP. You have two options:
  1. **Point the device to the Render HTTPS URL** — SIM800L can handle HTTPS with `AT+CHTTPSSTART` on newer firmware.
  2. **Use a plain HTTP proxy** if your SIM800L firmware doesn't support TLS. A lightweight VPS running Nginx can forward plain HTTP → HTTPS Render endpoint.
- The Render free tier does **not** expose a plain HTTP port — all traffic goes via their SSL termination.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `401 Unauthorized` | Check `x-api-key` header matches `API_KEY` env var on Render |
| `400 Bad Request` | Check JSON payload has valid `bin_id` (string), `lat` and `lng` (numbers) |
| `429 Too Many Requests` | ESP32 is sending too fast — rate limit is 20 req/min per IP |
| Supabase errors in logs | Verify `SUPABASE_URL` and `SUPABASE_SERVICE_ROLE_KEY` are correct |
| Server sleeping on free tier | Render free tier spins down after 15 min of inactivity — first request takes ~30s to wake up |
