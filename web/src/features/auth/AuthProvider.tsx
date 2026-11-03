import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from 'react'
import axios from 'axios'
import { getCurrentUser, login, register, signOut, type AuthUser, type LoginInput, type RegisterInput } from '../../api/auth'

type AuthStatus = 'loading' | 'anonymous' | 'authenticated'

type AuthContextValue = {
    user: AuthUser | null
    status: AuthStatus
    login: (input: LoginInput) => Promise<void>
    register: (input: RegisterInput) => Promise<void>
    signOut: () => Promise<void>
    refreshUser: () => Promise<void>
}

const AuthContext = createContext<AuthContextValue | null>(null)

export function AuthProvider({ children }: { children: ReactNode }) {
    const [user, setUser] = useState<AuthUser | null>(null)
    const [status, setStatus] = useState<AuthStatus>('loading')

    // 刷新使用者資訊
    const refreshUser = useCallback(async () => {
        try {
            const currentUser = await getCurrentUser()
            setUser(currentUser)
            setStatus('authenticated')
        } catch (error: unknown) {
            if (!axios.isAxiosError(error) || error.response?.status !== 401) {
                console.error('Unable to read the current user', error)
            }
            setUser(null)
            setStatus('anonymous')
        }
    }, [])

    useEffect(() => {
        void refreshUser()
    }, [refreshUser])

    const value = useMemo<AuthContextValue>(() => ({
        user,
        status,
        login: async (input) => {
            const currentUser = await login(input)
            setUser(currentUser)
            setStatus('authenticated')
        },
        register: async (input) => {
            const currentUser = await register(input)
            setUser(currentUser)
            setStatus('authenticated')
        },
        signOut: async () => {
            await signOut()
            setUser(null)
            setStatus('anonymous')
        },
        refreshUser,
    }), [refreshUser, status, user])

    return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

// 自訂 Hook，提供登入狀態與使用者資訊。
export function useAuth() {
    const context = useContext(AuthContext)
    if (context === null) {
        throw new Error('useAuth must be used within AuthProvider')
    }
    return context
}
