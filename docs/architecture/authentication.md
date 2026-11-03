# Authentication roadmap

## Implement now

- Email is the only account identifier.
- Email/password registration and login use Argon2 password hashes.
- Browser login uses a revocable, HttpOnly session cookie.
- New users receive the `user` role. Only existing administrators may assign `admin`.
- `/login` and `/register` are separate pages; the forgot-password control is visible but inactive.

## Deferred work

- Verify Email addresses before enabling sensitive actions.
- Password-reset Email flow and reset tokens.
- Google and GitHub account linking through a `user_identities` table.
- Login rate limits, audit logs, session management UI, and optional two-factor authentication.

## Security rules

- Never save or log original passwords.
- Return the same login error for an unknown Email and a wrong password.
- Never accept a role from public registration input.
- Keep session tokens out of frontend JavaScript and store only their hashes in the database.

## Dependency-injection decision

- For now, API routes call the small set of required service functions directly.
- Services and authentication dependencies may use `auth_repository` directly; this keeps the first Email/password implementation easy to read.
- Keep FastAPI `Depends` at the API boundary for concerns such as the current user, roles, and database sessions. Do not couple service functions to FastAPI.
- Introduce constructor-injected service and repository interfaces when password reset, Email verification, an admin area, or isolated unit tests make replacing repositories worthwhile.
