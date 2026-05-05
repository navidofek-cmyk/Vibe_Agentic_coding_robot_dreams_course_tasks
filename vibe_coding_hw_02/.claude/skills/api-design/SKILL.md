---
name: api-design
description: REST API design pravidla — použij při návrhu nových endpointů, URL struktur nebo HTTP response kódů.
---

# REST API Design Guidelines

## URL struktura

```
# Správně — resource noun, plural, hierarchie
GET    /users              # seznam
GET    /users/{id}         # detail
POST   /users              # vytvoření
PUT    /users/{id}         # celá aktualizace
PATCH  /users/{id}         # částečná aktualizace
DELETE /users/{id}         # smazání

GET    /users/{id}/orders  # vnořený resource

# Špatně — slovesa v URL
GET /getUsers
POST /createUser
GET /user/delete/1
```

## HTTP status kódy

| Situace | Kód |
|---------|-----|
| Vytvoření | 201 Created |
| Úspěch bez obsahu | 204 No Content |
| Validační chyba | 422 Unprocessable Entity |
| Nenalezeno | 404 Not Found |
| Neautorizován | 401 Unauthorized |
| Zakázáno | 403 Forbidden |
| Konflikt (duplicita) | 409 Conflict |

## Response formát

```json
// Seznam s paginací
{
  "items": [...],
  "total": 100,
  "page": 1,
  "size": 20
}

// Chyba
{
  "detail": "User not found"
}
```

## Verzování

- URL prefix: `/api/v1/users`
- Nikdy neměň existující endpoint — přidej novou verzi

## Idempotence

- GET, PUT, DELETE musí být idempotentní
- POST není idempotentní — každé volání vytvoří nový záznam
