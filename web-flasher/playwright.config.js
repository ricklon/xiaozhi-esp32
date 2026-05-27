module.exports = {
  testDir: "./tests",
  use: {
    browserName: "chromium",
    channel: "chrome",
    baseURL: "http://127.0.0.1:8080",
  },
  webServer: {
    command: "UV_CACHE_DIR=.uv-cache uv run python serve.py 8080",
    url: "http://127.0.0.1:8080",
    reuseExistingServer: true,
    timeout: 5000,
  },
};
