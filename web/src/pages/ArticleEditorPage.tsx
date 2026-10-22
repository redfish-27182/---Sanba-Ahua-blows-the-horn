import { useState } from 'react'
import type { Editor } from '@tiptap/react'
import axios from 'axios'
import { getArticle, saveArticle } from '../api/articles'
import ArticleEditor from '../features/faq/ArticleEditor'

// 本機暫時編輯頁；不屬於正式後台或身分驗證功能。
export default function ArticleEditorPage() {
    const [slug, setSlug] = useState('getting-started')
    const [title, setTitle] = useState('')
    const [editor, setEditor] = useState<Editor | null>(null)
    const [message, setMessage] = useState('')
    const [busy, setBusy] = useState(false)

    // 依 slug 讀回已有文章；載入會取代目前編輯內容。
    async function load() {
        if (!editor || busy) return
        setBusy(true)
        try {
            const article = await getArticle(slug.trim())
            setTitle(article.title)
            editor.commands.setContent(article.content)
            setMessage('文章已載入。')
        } catch (error) {
            setMessage(axios.isAxiosError(error) && error.response?.status === 404 ? '找不到文章；可填寫內容後儲存。' : '讀取失敗，請確認後端和資料庫。')
        } finally { setBusy(false) }
    }

    // 相同 slug 會覆蓋既有文章；新的 slug 會建立文章。
    async function save() {
        if (!editor || busy) return
        if (!/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(slug.trim()) || !title.trim()) {
            setMessage('請填寫標題，網址名稱只能使用小寫英數及連字號。')
            return
        }
        setBusy(true)
        try {
            await saveArticle(slug.trim(), title.trim(), editor.getJSON())
            setMessage(`儲存成功，可前往 /faq/${slug.trim()} 查看。`)
        } catch (error) {
            if (axios.isAxiosError(error) && error.response?.status === 403) setMessage('寫入未啟用；請確認本機後端設定。')
            else if (axios.isAxiosError(error) && error.response?.status === 422) setMessage('文章格式或網址不符合要求。')
            else setMessage('儲存失敗，請確認後端和資料庫。')
        } finally { setBusy(false) }
    }

    return (
        <main className="mx-auto max-w-4xl space-y-5 px-4 py-8">
            <h1 className="text-2xl font-bold">文章編輯器（本機測試）</h1>
            <p className="text-sm text-slate-600">此頁沒有登入驗證，僅供本機開發。相同網址名稱儲存時會更新原文章。</p>
            <label className="block text-sm font-medium">網址名稱
                <input value={slug} onChange={(event) => setSlug(event.target.value)} className="mt-1 block w-full rounded border border-slate-300 bg-white p-2" placeholder="getting-started" />
            </label>
            <label className="block text-sm font-medium">文章標題
                <input value={title} onChange={(event) => setTitle(event.target.value)} maxLength={200} className="mt-1 block w-full rounded border border-slate-300 bg-white p-2" />
            </label>
            <ArticleEditor onReady={setEditor} />
            <div className="flex flex-wrap gap-3">
                <button type="button" disabled={busy || !editor} onClick={load} className="rounded bg-slate-200 px-4 py-2 disabled:opacity-50">讀取文章</button>
                <button type="button" disabled={busy || !editor} onClick={save} className="rounded bg-blue-700 px-4 py-2 text-white disabled:opacity-50">儲存文章</button>
            </div>
            <p role="status" className="text-sm text-slate-700">{message}</p>
        </main>
    )
}
