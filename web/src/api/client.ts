import axios from 'axios'

// 網址可透過 VITE_API_BASE_URL 設定；前端環境變數不可存放密碼。
// 第一版只建立共用設定，不主動向後端發送請求。
const apiClient = axios.create({
    baseURL: import.meta.env.VITE_API_BASE_URL || undefined,
    timeout: 10000,
})

export default apiClient
