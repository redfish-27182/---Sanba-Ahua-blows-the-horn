import type { FaqArticleData } from './faqArticles'

// 顯示一篇指南文章；目前內容使用前端示範資料。
function FaqArticle({ article }: { article: FaqArticleData }) {
    return (
        <article className="min-w-0 flex-1 bg-white px-6 py-8 sm:px-10 sm:py-10">
            <p className="mb-3 text-sm font-medium text-blue-700">
                使用者指南
            </p>
            <h1 className="text-3xl font-bold tracking-tight text-slate-900">
                {article.title}
            </h1>
            <p className="mt-5 max-w-3xl leading-7 text-slate-600">
                {article.description}
            </p>
            <hr className="my-8 border-slate-200" />
            
            <div className="max-w-3xl space-y-9">
                {article.sections.map((section) => (
                    <section key={section.heading}>
                        <h2 className="border-b border-slate-200 pb-3 text-xl font-semibold text-slate-900">{section.heading}</h2>
                        {section.paragraphs.map((paragraph) => <p key={paragraph} className="mt-4 leading-8 text-slate-600">{paragraph}</p>)}
                    </section>
                ))}
            </div>
        </article>
    )
}

export default FaqArticle
