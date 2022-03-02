import { Express } from 'express';
import { createProxyMiddleware } from "http-proxy-middleware";

module.exports = function (app : Express) {
    app.use(createProxyMiddleware("/api", {target: "http://localhost:3002", changeOrigin: true})
    );
};