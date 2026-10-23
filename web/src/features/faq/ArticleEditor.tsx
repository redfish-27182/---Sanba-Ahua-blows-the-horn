import { EditorContent, useEditor } from '@tiptap/react'
import type { Editor, JSONContent } from '@tiptap/react'
import Image from '@tiptap/extension-image'
import StarterKit from '@tiptap/starter-kit'

type ArticleEditorProps = { onReady: (editor: Editor) => void }

// 提供基本文章格式；圖片以網路網址插入，不上傳檔案。
export default function ArticleEditor({ onReady }: ArticleEditorProps) {
    const editor = useEditor({
        extensions: [StarterKit.configure({ link: { openOnClick: false } }), Image],
        content: { type: 'doc', content: [{ type: 'paragraph' }] } satisfies JSONContent,
        onCreate: ({ editor: instance }) => onReady(instance),
        editorProps: { attributes: { class: 'article-content min-h-64 rounded-b-lg border border-slate-300 p-5 focus:outline-blue-500' } },
    })

    if (!editor) return <p>編輯器載入中...</p>

    // 只接受公開 HTTP(S) 圖片網址，避免使用暫時性本機網址。
    function insertImage() {
        const raw = window.prompt('請輸入圖片的 HTTP 或 HTTPS 網址：')
        if (!raw) return
        try {
            const url = new URL(raw)
            if (!['http:', 'https:'].includes(url.protocol) || url.username || url.password) throw new Error()
            editor?.chain().focus().setImage({ src: url.href }).run()
        } catch {
            window.alert('請輸入有效的 HTTP 或 HTTPS 網址。')
        }
    }

    const buttonClass = 'rounded border border-slate-300 bg-white px-3 py-1.5 text-sm hover:bg-slate-100'
    return (
        <div>
            <div className="flex flex-wrap gap-2 rounded-t-lg border border-b-0 border-slate-300 bg-slate-50 p-2" aria-label="文章格式工具列">
                <button 
                    type="button" 
                    className={buttonClass} 
                    onClick={() => editor.chain().focus().setParagraph().run()}
                >段落</button>
                
                <button 
                    type="button" 
                    className={buttonClass} 
                    onClick={() => editor.chain().focus().toggleHeading({ level: 2 }).run()}
                >標題</button>

                <button 
                    type="button" 
                    className={buttonClass} 
                    aria-pressed={editor.isActive('bold')} 
                    onClick={() => editor.chain().focus().toggleBold().run()}
                >粗體</button>

                <button 
                    type="button" 
                    className={buttonClass} 
                    aria-pressed={editor.isActive('italic')} 
                    onClick={() => editor.chain().focus().toggleItalic().run()}
                >斜體</button>

                <button 
                    type="button" 
                    className={buttonClass} 
                    onClick={() => editor.chain().focus().toggleBulletList().run()}
                >列表</button>

                <button 
                    type="button" 
                    className={buttonClass} 
                    onClick={() => editor.chain().focus().setHorizontalRule().run()}
                >分隔線</button>

                <button 
                    type="button" 
                    className={buttonClass} 
                    onClick={insertImage}
                >圖片網址</button>
            </div>
            <EditorContent editor={editor} />
        </div>
    )
}
