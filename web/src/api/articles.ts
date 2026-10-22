import type { JSONContent } from '@tiptap/react'
import apiClient from './client'

// 前端文章格式；資料庫實際欄位由後端負責。
export type Article = {
    slug: string
    title: string
    content: JSONContent
    created_at: string
    updated_at: string
}

// 依網址名稱向後端讀取文章。
export async function getArticle(slug: string, signal?: AbortSignal): Promise<Article> {
    const response = await apiClient.get<Article>(`/api/articles/${encodeURIComponent(slug)}`, { signal })
    return response.data
}

// 建立或更新文章；目前後端只允許本機測試。
export async function saveArticle(slug: string, title: string, content: JSONContent): Promise<Article> {
    const response = await apiClient.put<Article>(`/api/articles/${encodeURIComponent(slug)}`, { title, content })
    return response.data
}
