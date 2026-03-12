const { insertLocation, insertTelemetry } = require('../services/supabase.service');

/**
 * Middleware: validates the x-api-key header.
 * Used only on POST /update-location (legacy route).
 * POST /data does NOT require this — the Arduino sends no auth header.
 */
function requireApiKey(req, res, next) {
  const apiKey = req.headers['x-api-key'];
  if (!apiKey || apiKey !== process.env.API_KEY) {
    return res.status(401).json({ status: 'error', message: 'Unauthorized: invalid or missing x-api-key' });
  }
  next();
}

/**
 * POST /update-location  (legacy — kept for backward compatibility)
 * Expects: { bin_id, lat, lng }
 */
async function updateLocation(req, res, next) {
  const { bin_id, lat, lng } = req.body;

  if (!bin_id || typeof bin_id !== 'string' || bin_id.trim() === '') {
    return res.status(400).json({ status: 'error', message: 'bin_id must be a non-empty string' });
  }
  if (lat === undefined || lat === null || !isFinite(lat)) {
    return res.status(400).json({ status: 'error', message: 'lat must be a finite number' });
  }
  if (lng === undefined || lng === null || !isFinite(lng)) {
    return res.status(400).json({ status: 'error', message: 'lng must be a finite number' });
  }

  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] ${bin_id.trim()} → lat: ${lat}, lng: ${lng}`);

  try {
    await insertLocation(bin_id.trim(), lat, lng);
    return res.status(200).json({ status: 'success' });
  } catch (err) {
    next(err);
  }
}

/**
 * POST /data
 * Primary ingestion endpoint — matches saved_to_mongodb.ino payload exactly.
 *
 * Expected JSON body:
 *   {
 *     "device_id" : "bin-01",         // string — device identifier
 *     "lat"       : 23.892550,        // float  — GPS latitude
 *     "lng"       : 89.595650,        // float  — GPS longitude
 *     "distance"  : 12.50,            // float  — ultrasonic reading (cm)
 *     "bin_full"  : false,            // bool   — hardware full-flag
 *     "device_ts" : 45231             // number — device uptime ms (millis())
 *   }
 *
 * No API key required — Arduino HTTP client sends no auth header.
 * Rate limiting should be applied at the reverse-proxy / middleware level.
 *
 * SDMS integration note:
 *   fill_level is computed server-side from distance and stored so that
 *   the row is immediately usable by the main project's iot_readings
 *   trigger chain without any further transformation.
 */
async function ingestData(req, res, next) {
  const { device_id, lat, lng, distance, bin_full, device_ts } = req.body;

  // ── Validation ──────────────────────────────────────────────────────────
  if (!device_id || typeof device_id !== 'string' || device_id.trim() === '') {
    return res.status(400).json({ status: 'error', message: 'device_id must be a non-empty string' });
  }
  if (lat === undefined || lat === null || !isFinite(Number(lat))) {
    return res.status(400).json({ status: 'error', message: 'lat must be a finite number' });
  }
  if (lng === undefined || lng === null || !isFinite(Number(lng))) {
    return res.status(400).json({ status: 'error', message: 'lng must be a finite number' });
  }
  if (distance === undefined || distance === null || !isFinite(Number(distance)) || Number(distance) < 0) {
    return res.status(400).json({ status: 'error', message: 'distance must be a non-negative finite number' });
  }
  if (bin_full === undefined || bin_full === null || typeof bin_full !== 'boolean') {
    return res.status(400).json({ status: 'error', message: 'bin_full must be a boolean' });
  }

  // ── Log ─────────────────────────────────────────────────────────────────
  const timestamp = new Date().toISOString();
  console.log(
    `[${timestamp}] /data | device: ${device_id.trim()} | ` +
    `lat: ${lat}, lng: ${lng} | dist: ${distance} cm | full: ${bin_full}`
  );

  // ── Persist ─────────────────────────────────────────────────────────────
  try {
    await insertTelemetry({
      device_id: device_id.trim(),
      lat:       Number(lat),
      lng:       Number(lng),
      distance:  Number(distance),
      bin_full,
      device_ts: device_ts !== undefined ? Number(device_ts) : null,
    });
    return res.status(200).json({ status: 'success' });
  } catch (err) {
    next(err);
  }
}

/**
 * GET /health
 * Public health check — no auth required.
 */
function healthCheck(req, res) {
  res.status(200).json({
    status: 'ok',
    uptime: Math.floor(process.uptime()),
  });
}

module.exports = { updateLocation, ingestData, requireApiKey, healthCheck };
