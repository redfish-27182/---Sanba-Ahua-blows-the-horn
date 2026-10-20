import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router'
import './index.css'
import App from './App'

const rootElement = document.getElementById('root')

if (rootElement === null) {
    throw new Error('找不到 React 掛載節點。')
}

createRoot(rootElement).render(
    <StrictMode>
        <BrowserRouter>
            <App />
        </BrowserRouter>
    </StrictMode>,
)
