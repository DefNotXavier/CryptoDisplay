// Dashboard settings. Edit on the chip, then unplug/replug to apply.
// coin_ids are CoinGecko API IDs. Colors are hex.
const CONFIG = {
  title: "Crypto Dashboard v2.0 (Web)",
  coin_ids: ["bitcoin", "dogecoin", "solana", "ripple", "ethereum"],
  update_interval_current_seconds: 450,
  update_interval_historic_seconds: 86400,
  api_delay_ms: 1000,
  api_key: "",
  ui_colors: {
    header_bg: "#1e1e1e",
    row_bg: "#141414",
    gold: "#ffcb00",
  },
};
