const express = require('express');
const router  = express.Router();
const {
  updateLocation,
  ingestData,
  requireApiKey,
  healthCheck,
} = require('../controllers/location.controller');

/**
 * POST /data
 * Primary IoT ingestion endpoint — matches saved_to_mongodb.ino exactly.
 * Payload: { device_id, lat, lng, distance, bin_full, device_ts }
 * No API key required (Arduino sends no auth header).
 */
router.post('/data', ingestData);

/**
 * POST /update-location  (legacy)
 * Kept for backward compatibility.
 * Requires x-api-key header.
 */
router.post('/update-location', requireApiKey, updateLocation);

/**
 * GET /health
 * Public health check — no auth required.
 */
router.get('/health', healthCheck);

module.exports = router;
