import { Link, useParams } from 'react-router'
import { useEffect, useState } from 'react'
import axios from 'axios'
import { getArticle } from '../api/articles'
import type { Article } from '../api/articles'
import SidebarMenu from '../components/SidebarMenu'
import FaqArticle from '../features/faq/FaqArticle'
import { faqMenu } from '../features/faq/faqMenu'

// 根據網址選擇文章，並組合共用選單與文章顯示區。
export default function FaqPage() {
    const { articleSlug } = useParams() // 從網址取得文章 slug
    const [article, setArticle] = useState<Article | null>(null)
    const [status, setStatus] = useState<'loading' | 'ready' | 'missing' | 'error'>('loading')

    // 文章網址變更時重新讀取；取消舊請求以避免畫面顯示錯誤文章。
    useEffect(() => {
        if (!articleSlug) return
        const controller = new AbortController()
        setStatus('loading')
        setArticle(null)
        getArticle(articleSlug, controller.signal).then((result) => {
            setArticle(result)
            setStatus('ready')
        }).catch((error: unknown) => {
            if (controller.signal.aborted) return
            setStatus(axios.isAxiosError(error) && error.response?.status === 404 ? 'missing' : 'error')
        })
        return () => controller.abort()
    }, [articleSlug])

    return (
        <div className="mx-auto max-w-6xl px-4 py-6 sm:px-6 sm:py-10">

            {/* 頁面導航 */}
            <div className="mb-5 text-sm text-slate-500">
                <Link to="/" className="hover:text-blue-700">
                    首頁
                </Link>
                <span className="mx-2">
                    /
                </span>
                <Link to="/faq" className="hover:text-blue-700">
                    使用者指南
                </Link>
                {article && <><span className="mx-2">/</span><span className="text-slate-800">{article.title}</span></>}
            </div>
            
            {/* 主要內容 */}
            <div className="min-h-[32rem] overflow-visible rounded-xl border border-slate-200 bg-white shadow-sm sm:flex">
                
                {/* 自製側邊選單 */}
                <SidebarMenu items={faqMenu} label="指南目錄" />
                
                {status === 'ready' && 
                    article ? 
                        <FaqArticle article={article} /> 
                        : (
                        <div className="flex-1 px-6 py-10 sm:px-10">
                            {status === 'loading' && 
                                <p role="status">
                                    文章載入中...
                                </p>}

                            {status === 'missing' && 
                                <>
                                    <h1 className="text-2xl font-bold text-slate-900">
                                        找不到這篇文章
                                    </h1>
                                    <p className="mt-3 text-slate-600">
                                        這篇文章尚未建立，請從左側目錄選擇其他文章。
                                    </p>
                                    <Link to="/faq" className="mt-6 inline-block font-medium text-blue-700 hover:underline">
                                        返回使用者指南
                                    </Link>
                                </>
                            }
                            
                            {status === 'error' && 
                                <>
                                    <h1 className="text-2xl font-bold text-slate-900">
                                        無法讀取文章
                                    </h1>
                                    <p className="mt-3 text-slate-600">
                                        請確認後端與資料庫是否正常運作。
                                    </p>
                                </>
                            }
                        </div>
                )}
            </div>
        </div>
    )
}

