require('dotenv').config();

const express = require('express');
const cors = require('cors');
const rateLimit = require('express-rate-limit');
const locationRoutes = require('./routes/location.routes');
const errorHandler = require('./middleware/errorHandler');

// Initialize Express app
const app = express();

/**
 * Trust Railway's reverse proxy so that express-rate-limit sees the real
 * client IP (forwarded via X-Forwarded-For) rather than the internal
 * Railway router IP. Without this every request appears to come from the
 * same IP and all bins would share one rate-limit bucket.
 */
app.set('trust proxy', 1);

// Parse JSON request bodies
app.use(express.json());

// Enable CORS — ESP32 sends raw HTTP so no browser CORS enforcement,
// but this is needed if the main-project dashboard calls the API directly.
app.use(cors());

/**
 * Rate limiter: 20 requests per minute per IP.
 *
 * Design rationale:
 * - Each ESP32 bin sends 1 request/min (60-second interval).
 * - Multiple bins may share one external IP behind a GSM NAT gateway.
 * - max=20 accommodates up to 20 bins per shared IP while still blocking
 *   runaway devices or abuse (>20 req/min is clearly abnormal).
 * - windowMs=60s matches the device interval so the window resets cleanly.
 */
const limiter = rateLimit({
  windowMs: 60 * 1000,
  max: 20,
  standardHeaders: true,
  legacyHeaders: false,
  message: { status: 'error', message: 'Too many requests — device may be misconfigured or sending too fast.' },
});

app.use(limiter);

// Mount all routes
app.use('/', locationRoutes);

// Global error handler — must be last middleware
app.use(errorHandler);

// Start listening on Railway-provided PORT or fallback to 3000 locally
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`[${new Date().toISOString()}] Server running on port ${PORT}`);
});

