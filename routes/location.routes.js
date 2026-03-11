const express = require('express');
const router = express.Router();
const { updateLocation, requireApiKey, healthCheck } = require('../controllers/location.controller');

/**
 * POST /update-location
 * Requires x-api-key header. Validates payload and stores GPS data in Supabase.
 */
router.post('/update-location', requireApiKey, updateLocation);

/**
 * GET /health
 * Public health check endpoint — no auth required.
 */
router.get('/health', healthCheck);

module.exports = router;
