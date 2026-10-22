import axios from 'axios'

// 網址可透過 VITE_API_BASE_URL 設定；前端環境變數不可存放密碼。
const apiClient = axios.create({
    baseURL: import.meta.env.VITE_API_BASE_URL || (import.meta.env.DEV ? 'http://127.0.0.1:8000' : undefined),
    timeout: 10000,
})

export default apiClient
