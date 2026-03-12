-- ============================================================
-- Migration 001: Create bin_locations table
-- Run this in your Supabase SQL Editor (Project → SQL Editor)
-- ============================================================

CREATE TABLE IF NOT EXISTS bin_locations (
  id          BIGINT        GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  bin_id      TEXT          NOT NULL,                  -- device_id from the ESP32 (e.g. "bin-01")
  lat         NUMERIC(10,7) NOT NULL,                  -- GPS latitude
  lng         NUMERIC(10,7) NOT NULL,                  -- GPS longitude
  distance    NUMERIC(8,2),                            -- HC-SR04 reading in cm
  bin_full    BOOLEAN,                                 -- hardware full-flag from device
  device_ts   BIGINT,                                  -- device uptime in ms (millis())
  fill_level  INTEGER CHECK (fill_level BETWEEN 0 AND 100),  -- derived 0-100 %
  raw_payload TEXT,                                    -- full JSON blob from device
  created_at  TIMESTAMPTZ   NOT NULL DEFAULT now()
);

-- Indexes for fast lookup
CREATE INDEX IF NOT EXISTS idx_bin_locations_bin_id  ON bin_locations (bin_id);
CREATE INDEX IF NOT EXISTS idx_bin_locations_created ON bin_locations (created_at DESC);

-- ============================================================
-- SDMS Integration Bridge (run AFTER you set up the main project)
-- ============================================================
-- When you are ready to integrate, run the PostgreSQL function below.
-- It fires after every INSERT on bin_locations and copies the reading
-- into the main project's iot_readings table so that sync_fill_level
-- trigger can auto-update the dustbins.fill_level column.
--
-- Prerequisites:
--   1. The SDMS schema (schema.sql) must already be applied.
--   2. dustbins rows must have device_id values matching bin_id here.
--
-- CREATE OR REPLACE FUNCTION bridge_to_iot_readings()
-- RETURNS TRIGGER
-- SET search_path = public
-- AS $$
-- DECLARE
--   v_bin_uuid UUID;
-- BEGIN
--   -- Lookup the dustbins.id by device_id
--   SELECT id INTO v_bin_uuid
--   FROM dustbins
--   WHERE device_id = NEW.bin_id
--   LIMIT 1;
--
--   IF v_bin_uuid IS NOT NULL AND NEW.fill_level IS NOT NULL THEN
--     INSERT INTO iot_readings (bin_id, fill_level)
--     VALUES (v_bin_uuid, NEW.fill_level);
--   END IF;
--
--   RETURN NEW;
-- END;
-- $$ LANGUAGE plpgsql;
--
-- DROP TRIGGER IF EXISTS after_bin_location_insert ON bin_locations;
-- CREATE TRIGGER after_bin_location_insert
-- AFTER INSERT ON bin_locations
-- FOR EACH ROW EXECUTE FUNCTION bridge_to_iot_readings();
