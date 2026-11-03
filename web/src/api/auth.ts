import apiClient from './client'

export type AuthUser = {
    id: number
    email: string
    role: 'user' | 'admin'
}

export type LoginInput = {
    email: string
    password: string
}

export type RegisterInput = LoginInput & {
    confirmPassword: string
}

export async function getCurrentUser() {
    const { data } = await apiClient.get<AuthUser>('/auth/me')
    return data
}

export async function login(input: LoginInput) {
    const { data } = await apiClient.post<AuthUser>('/auth/login', input)
    return data
}

export async function register(input: RegisterInput) {
    const { data } = await apiClient.post<AuthUser>('/auth/register', {
        email: input.email,
        password: input.password,
        confirm_password: input.confirmPassword,
    })
    return data
}

export async function signOut() {
    await apiClient.post('/auth/logout')
}
