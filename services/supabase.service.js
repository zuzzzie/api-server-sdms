// Service layer for all Supabase database operations
const supabase = require('../config/supabaseClient');

/**
 * Inserts a new bin location record into the bin_locations table.
 * Legacy function — kept for backward compat with POST /update-location.
 *
 * @param {string} bin_id
 * @param {number} lat
 * @param {number} lng
 */
async function insertLocation(bin_id, lat, lng) {
  const { data, error } = await supabase
    .from('bin_locations')
    .insert([{ bin_id, lat, lng }]);

  if (error) throw new Error(`Supabase insert failed: ${error.message}`);
  return data;
}

/**
 * Derives a fill_level percentage (0–100) from the raw ultrasonic distance.
 *
 * The HC-SR04 measures the air gap from sensor → waste surface.
 * Smaller distance = bin more full.
 *
 * Calibration constants (matching saved_to_mongodb.ino):
 *   FULL_CM  = 10 → distance ≤ 10 cm  → 100 %
 *   EMPTY_CM = 50 → distance ≥ 50 cm  →   0 %
 *
 *   fill_level = clamp( (EMPTY_CM - dist) / (EMPTY_CM - FULL_CM) * 100, 0, 100 )
 *
 * @param {number}  distanceCm
 * @param {boolean} binFull    - hardware flag (dist < FULL_CM)
 * @returns {number} integer 0–100
 */
function deriveFillLevel(distanceCm, binFull) {
  if (binFull) return 100;
  const EMPTY_CM = 50;
  const FULL_CM  = 10;
  const raw = ((EMPTY_CM - distanceCm) / (EMPTY_CM - FULL_CM)) * 100;
  return Math.round(Math.min(100, Math.max(0, raw)));
}

/**
 * Inserts a full IoT telemetry record into bin_locations.
 * Called by POST /data — matches the payload from saved_to_mongodb.ino exactly.
 *
 * Device payload:
 *   { device_id, lat, lng, distance, bin_full, device_ts }
 *
 * Main-project (SDMS) compatibility notes:
 *   - bin_id     → stored as text matching dustbins.device_id
 *   - fill_level → derived here; mirrors iot_readings.fill_level & triggers
 *                  the sync_fill_level trigger in the main project schema
 *   - raw_payload → full JSON blob; mirrors telemetry.raw_payload in SDMS
 *   - When integrating: a Supabase trigger / Edge Function can bridge this
 *     table to iot_readings by looking up dustbins.id via device_id.
 *
 * @param {object} payload
 */
async function insertTelemetry({ device_id, lat, lng, distance, bin_full, device_ts }) {
  const fill_level = deriveFillLevel(distance, bin_full);

  const record = {
    bin_id:      device_id,  // maps to dustbins.device_id in SDMS
    lat,
    lng,
    distance,
    bin_full,
    device_ts,
    fill_level,
    raw_payload: JSON.stringify({ device_id, lat, lng, distance, bin_full, device_ts }),
  };

  const { data, error } = await supabase
    .from('bin_locations')
    .insert([record]);

  if (error) throw new Error(`Supabase insert failed: ${error.message}`);
  return data;
}

module.exports = { insertLocation, insertTelemetry, deriveFillLevel };
