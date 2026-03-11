/**
 * Global Express error-handling middleware.
 * Catches all errors passed via next(err) and returns a structured JSON error response.
 * Ensures the server never crashes due to unhandled errors.
 */
function errorHandler(err, req, res, next) {
  console.error(`[ERROR] ${new Date().toISOString()} - ${err.message}`);

  const statusCode = err.statusCode || 500;
  res.status(statusCode).json({
    status: 'error',
    message: err.message || 'Internal server error',
  });
}

module.exports = errorHandler;
