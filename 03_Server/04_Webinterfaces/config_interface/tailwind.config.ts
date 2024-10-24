import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
    "./src/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    colors: {
      danube: {
        50: "#f2f7fc",
        100: "#e1eef8",
        200: "#cae2f3",
        300: "#a6d0ea",
        400: "#7bb6df",
        500: "#5b9bd5",
        600: "#4883c8",
        700: "#3e70b7",
        800: "#375c96",
        900: "#314d77",
        950: "#223149",
      },
      white: "#ffffff",
      transparent: "transparent",
      red: "#963748",
      black: "#000000",
    },
    extend: {
      backgroundImage: {
        "gradient-radial": "radial-gradient(var(--tw-gradient-stops))",
        "gradient-conic": "conic-gradient(from 180deg at 50% 50%, var(--tw-gradient-stops))",
      },
    },
  },
  plugins: [require("tailwind-scrollbar-hide")],
};
export default config;
