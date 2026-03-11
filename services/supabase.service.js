// Service layer for all Supabase database operations
const supabase = require('../config/supabaseClient');

/**
 * Inserts a new bin location record into the bin_locations table.
 * @param {string} bin_id - The identifier for the dustbin
 * @param {number} lat - Latitude coordinate
 * @param {number} lng - Longitude coordinate
 * @returns {Promise<object>} The inserted data or throws on error
 */
async function insertLocation(bin_id, lat, lng) {
  const { data, error } = await supabase
    .from('bin_locations')
    .insert([{ bin_id, lat, lng }]);

  if (error) {
    throw new Error(`Supabase insert failed: ${error.message}`);
  }

  return data;
}

module.exports = { insertLocation };
