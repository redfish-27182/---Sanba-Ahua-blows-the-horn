import { LoaderCircle, UserPlus } from 'lucide-react'
import { Link, Navigate, useNavigate } from 'react-router'
import { useState, type FormEvent } from 'react'
import axios from 'axios'
import { useAuth } from '../features/auth/AuthProvider'

function getErrorMessage(error: unknown) {
    if (axios.isAxiosError<{ detail?: string }>(error)) {
        return error.response?.data.detail ?? '暫時無法註冊，請稍後再試。'
    }
    return '暫時無法註冊，請稍後再試。'
}

export default function RegisterPage() {
    const navigate = useNavigate()
    const { user, register } = useAuth()
    const [email, setEmail] = useState('')
    const [password, setPassword] = useState('')
    const [confirmPassword, setConfirmPassword] = useState('')
    const [errorMessage, setErrorMessage] = useState<string | null>(null)
    const [isSubmitting, setIsSubmitting] = useState(false)

    if (user !== null) {
        return <Navigate to="/" replace />
    }

    // 註冊表單提交
    async function submit(event: FormEvent<HTMLFormElement>) {
        event.preventDefault()
        setErrorMessage(null)
        setIsSubmitting(true)
        try {
            await register({ email, password, confirmPassword })
            navigate('/', { replace: true })
        } catch (error: unknown) {
            setErrorMessage(getErrorMessage(error))
        } finally {
            setIsSubmitting(false)
        }
    }

    return (
        <section className="grid min-h-[calc(100svh-4.75rem)] place-items-center px-4 py-10">
            <form 
                onSubmit={submit} 
                className="w-full max-w-md rounded-[2rem] border border-white/35 bg-white/12 p-8 
                            text-[var(--page-foreground)] shadow-2xl backdrop-blur-sm sm:p-10"
            >
                <p className="text-sm font-black tracking-[0.22em] opacity-70">NEW ACCOUNT</p>
                <h1 className="mt-3 text-4xl font-black tracking-tight">註冊</h1>
                <p className="mt-3 text-sm leading-6 opacity-80">先用 Email 與密碼建立帳號，基本資料之後再補上。</p>

                <label className="mt-7 block text-sm font-bold" htmlFor="register-email">
                    Email
                </label>

                <input 
                    id="register-email" 
                    type="email" 
                    autoComplete="email" 
                    required 
                    value={email} 
                    onChange={(event) => setEmail(event.target.value)} 
                    className="mt-2 w-full rounded-xl border border-white/45 bg-white/10 px-4 py-3 
                                text-[var(--page-foreground)] outline-none 
                                focus:border-white focus:ring-2 focus:ring-white/40" 
                />

                <label className="mt-5 block text-sm font-bold" htmlFor="register-password">
                    密碼
                </label>

                <input 
                    id="register-password" 
                    type="password" 
                    autoComplete="new-password" 
                    minLength={12} 
                    required 
                    value={password} 
                    onChange={(event) => setPassword(event.target.value)} 
                    className="mt-2 w-full rounded-xl border border-white/45 bg-white/10 px-4 py-3 
                                text-[var(--page-foreground)] outline-none 
                                focus:border-white focus:ring-2 focus:ring-white/40" 
                />

                <label className="mt-5 block text-sm font-bold" htmlFor="register-confirm-password">
                    確認密碼
                </label>

                <input 
                    id="register-confirm-password" 
                    type="password" 
                    autoComplete="new-password" 
                    minLength={12} 
                    required 
                    value={confirmPassword} 
                    onChange={(event) => setConfirmPassword(event.target.value)} 
                    className="mt-2 w-full rounded-xl border border-white/45 bg-white/10 px-4 py-3 
                            text-[var(--page-foreground)] outline-none 
                            focus:border-white focus:ring-2 focus:ring-white/40" 
                />

                {errorMessage && 
                    <p role="alert" className="mt-4 rounded-xl bg-red-950/25 px-4 py-3 text-sm font-bold">
                        {errorMessage}
                    </p>}

                <button 
                    type="submit" 
                    disabled={isSubmitting} 
                    className="mt-7 flex w-full items-center justify-center gap-3 rounded-full bg-emerald-400 px-5 py-3 font-black 
                            text-emerald-950 shadow-sm transition-transform hover:-translate-y-0.5 
                            hover:bg-emerald-300 disabled:cursor-wait disabled:opacity-70 
                            focus-visible:outline-2 focus-visible:outline-offset-4 focus-visible:outline-white"
                >
                    {isSubmitting ? 
                        <LoaderCircle className="animate-spin" aria-hidden="true" /> : <UserPlus aria-hidden="true" />}
                    <span>{isSubmitting ? '建立中…' : '建立帳號'}</span>
                </button>

                <p className="mt-6 text-center text-sm opacity-80">
                    已有帳號？
                    <Link to="/login" className="font-black underline underline-offset-4 hover:opacity-75">
                        登入
                    </Link>
                </p>
            </form>
        </section>
    )
}
