## Here's your optimized prompt:

---

You are a senior backend engineer building a production-ready IoT API server.

**Project Overview:**
Build a lightweight Node.js + Express API server that receives HTTP POST requests from an ESP32 + SIM800L Arduino device installed in a smart dustbin. The device periodically sends GPS coordinates. The server validates, logs, and forwards this data to a Supabase database.

---

**Device Behavior (read carefully):**
- The Arduino device sends raw HTTP (not HTTPS) POST requests
- Payload is JSON: `{ "bin_id": "BIN_01", "lat": 13.0827, "lng": 80.2707 }`
- Requests come from a SIM800L GSM module with limited retry logic
- The device sends one request every 30 seconds per bin
- There may be multiple bins running simultaneously

---

**Technology Stack (use exactly these):**
- Runtime: Node.js
- Framework: Express.js
- Database client: @supabase/supabase-js
- Middleware: cors, dotenv, express-rate-limit
- No TypeScript — plain JavaScript only

---

**Supabase Table Schema:**
Table name: `bin_locations`
| Column | Type | Notes |
|---|---|---|
| id | uuid | primary key, default gen_random_uuid() |
| bin_id | text | not null |
| lat | float8 | not null |
| lng | float8 | not null |
| created_at | timestamptz | default now() |

---

**Project Folder Structure (strict):**
```
project-root/
├── server.js
├── .env
├── .env.example
├── routes/
│   └── location.routes.js
├── controllers/
│   └── location.controller.js
├── services/
│   └── supabase.service.js
├── config/
│   └── supabaseClient.js
├── middleware/
│   └── errorHandler.js
```

---

**Required Endpoints:**

`POST /update-location`
- Accept JSON body
- Validate: `bin_id` must be a non-empty string, `lat` and `lng` must be finite numbers
- On invalid input: return HTTP 400 with `{ "status": "error", "message": "<reason>" }`
- On success: insert into Supabase, return HTTP 200 with `{ "status": "success" }`
- Log to console: `[2024-01-01T10:00:00Z] BIN_01 → lat: 13.0827, lng: 80.2707`

`GET /health`
- Returns HTTP 200 with `{ "status": "ok", "uptime": <seconds> }`
- Used by Render for health checks

---

**Security & Reliability Rules:**
- Every request must include header `x-api-key` matching `API_KEY` in `.env` — reject with HTTP 401 if missing or wrong
- Rate limit: maximum 20 requests per minute per IP using `express-rate-limit`
- All Supabase errors must be caught and passed to the error handler — never crash the server
- Error handler middleware must return `{ "status": "error", "message": "..." }` for all failures

---

**Environment Variables:**
```
PORT=3000
SUPABASE_URL=your_supabase_project_url
SUPABASE_SERVICE_ROLE_KEY=your_service_role_key
API_KEY=your_secret_key_for_devices
```

---

**Code Quality Rules:**
- Add a comment above every function explaining what it does
- Use `async/await` — no raw `.then()` chains
- Keep Supabase logic strictly inside `services/supabase.service.js`
- Keep request validation strictly inside the controller
- `server.js` must only wire things together — no business logic

---

**Deliverables (produce all of these):**
1. Every file in the folder structure above with complete, working code
2. `.env.example` with placeholder values
3. `package.json` with all dependencies and a `"start": "node server.js"` script
4. A sample `curl` command to test the endpoint locally
5. Step-by-step Render deployment guide including:
   - How to set environment variables on Render dashboard
   - Which start command to use
   - How to verify the health check passes
   - Note about HTTP vs HTTPS on Render's free tier

---

**Start by generating the files in this order:**
`config/supabaseClient.js` → `services/supabase.service.js` → `middleware/errorHandler.js` → `controllers/location.controller.js` → `routes/location.routes.js` → `server.js` → `.env.example` → `package.json` → deployment guide
