import { useEffect } from 'react'
import { EditorContent, useEditor } from '@tiptap/react'
import Image from '@tiptap/extension-image'
import StarterKit from '@tiptap/starter-kit'
import type { Article } from '../../api/articles'

// 以唯讀模式顯示後端儲存的 Tiptap 文件。
export default function FaqArticle({ article }: { article: Article }) {
    const editor = useEditor({
        extensions: [StarterKit, Image],
        content: article.content,
        editable: false,
        editorProps: { 
            attributes: { 
                class: 'article-content max-w-3xl' 
            } 
        },
    })

    // 網址切換文章時更新內容，不重新建立畫面元件。
    useEffect(() => {
        editor?.commands.setContent(article.content)
    }, [editor, article.content])

    return (
        <article className="min-w-0 flex-1 bg-white px-6 py-8 sm:px-10 sm:py-10">
            <p className="mb-3 text-sm font-medium text-blue-700">
                使用者指南
            </p>
            <h1 className="text-3xl font-bold tracking-tight text-slate-900">
                {article.title}
            </h1>
            <hr className="my-8 border-slate-200" />

            {/* 文章內容 */}
            <EditorContent editor={editor} />
        </article>
    )
}
