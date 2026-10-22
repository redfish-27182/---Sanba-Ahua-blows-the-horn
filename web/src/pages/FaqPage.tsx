import { Link, useParams } from 'react-router'
import SidebarMenu from '../components/SidebarMenu'
import FaqArticle from '../features/faq/FaqArticle'
import { faqArticles } from '../features/faq/faqArticles'
import { faqMenu } from '../features/faq/faqMenu'

// 根據網址選擇文章，並組合共用選單與文章顯示區。
export default function FaqPage() {
    const { articleSlug } = useParams() // 從網址取得文章 slug
    const article = faqArticles.find((item) => item.slug === articleSlug) // 根據網址 slug 找到對應文章

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
                
                <SidebarMenu items={faqMenu} label="指南目錄" />
                
                {article ? <FaqArticle article={article} /> : (

                    <div className="flex-1 px-6 py-10 sm:px-10">
                        <h1 className="text-2xl font-bold text-slate-900">
                            找不到這篇文章
                        </h1>
                        <p className="mt-3 text-slate-600">
                            請確認網址，或從左側目錄選擇其他文章。
                        </p>
                        <Link to="/faq" className="mt-6 inline-block font-medium text-blue-700 hover:underline">
                            返回使用者指南
                        </Link>
                    </div>
                )}
            </div>
        </div>
    )
}

