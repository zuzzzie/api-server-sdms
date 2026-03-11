require('dotenv').config();

const express = require('express');
const cors = require('cors');
const rateLimit = require('express-rate-limit');
const locationRoutes = require('./routes/location.routes');
const errorHandler = require('./middleware/errorHandler');

// Initialize Express app
const app = express();

// Parse JSON request bodies
app.use(express.json());

// Enable CORS for all origins (ESP32 sends raw HTTP — no CORS enforcement from device side)
app.use(cors());

/**
 * Rate limiter: maximum 20 requests per minute per IP.
 * Protects against abuse and accidental device misconfiguration.
 */
const limiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 20,
  standardHeaders: true,
  legacyHeaders: false,
  message: { status: 'error', message: 'Too many requests, please try again later.' },
});

app.use(limiter);

// Mount all routes
app.use('/', locationRoutes);

// Global error handler — must be last middleware
app.use(errorHandler);

// Start listening
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`[${new Date().toISOString()}] Server running on port ${PORT}`);
});
