const { insertLocation } = require('../services/supabase.service');

/**
 * Middleware to validate and authenticate x-api-key header.
 * Rejects requests with missing or incorrect API keys with HTTP 401.
 */
function requireApiKey(req, res, next) {
  const apiKey = req.headers['x-api-key'];

  if (!apiKey || apiKey !== process.env.API_KEY) {
    return res.status(401).json({ status: 'error', message: 'Unauthorized: invalid or missing x-api-key' });
  }

  next();
}

/**
 * Controller for POST /update-location
 * Validates the incoming JSON payload, logs the event, and persists data to Supabase.
 * Returns HTTP 400 on validation failure, HTTP 200 on success.
 */
async function updateLocation(req, res, next) {
  const { bin_id, lat, lng } = req.body;

  // Validate bin_id
  if (!bin_id || typeof bin_id !== 'string' || bin_id.trim() === '') {
    return res.status(400).json({ status: 'error', message: 'bin_id must be a non-empty string' });
  }

  // Validate lat and lng
  if (lat === undefined || lat === null || !isFinite(lat)) {
    return res.status(400).json({ status: 'error', message: 'lat must be a finite number' });
  }

  if (lng === undefined || lng === null || !isFinite(lng)) {
    return res.status(400).json({ status: 'error', message: 'lng must be a finite number' });
  }

  // Log the incoming data in the required format
  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] ${bin_id.trim()} → lat: ${lat}, lng: ${lng}`);

  try {
    await insertLocation(bin_id.trim(), lat, lng);
    return res.status(200).json({ status: 'success' });
  } catch (err) {
    // Pass to global error handler
    next(err);
  }
}

/**
 * Controller for GET /health
 * Returns server uptime in seconds for health check purposes (e.g. Render).
 */
function healthCheck(req, res) {
  res.status(200).json({
    status: 'ok',
    uptime: Math.floor(process.uptime()),
  });
}

module.exports = { updateLocation, requireApiKey, healthCheck };
