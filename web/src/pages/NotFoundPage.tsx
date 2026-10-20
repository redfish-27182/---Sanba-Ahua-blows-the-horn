import { Link } from 'react-router'

function NotFoundPage() {
    return (
        <div className="mx-auto max-w-6xl px-4 py-16">
            <h1 className="text-2xl font-bold">找不到這個頁面</h1>
            <Link to="/" className="mt-4 inline-block text-blue-700 underline">返回首頁</Link>
        </div>
    )
}

export default NotFoundPage
