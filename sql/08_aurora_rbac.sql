-- ============================================================================
-- PROJECT AURORA v7.1.0 - RBAC permissions for Aurora commands
-- Target: AzerothCore master (3.3.5a), AUTH database
-- Load AFTER the schema. Idempotent: safe to re-run.
--
-- Ids 1050-1051 are inside the module-owned range and do not collide with
-- the core set. Permissions are NOT granted to any default security level,
-- so neither `.aurora vote cast` nor `.aurora shop buy` can be used until an
-- administrator grants them deliberately (see comments at the end).
-- ============================================================================

DELETE FROM `rbac_permissions` WHERE `id` IN (1050, 1051);
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
  (1050, 'Command: aurora vote cast'),
  (1051, 'Command: aurora shop buy');

-- Revoke from every role first so the state is deterministic on re-run.
DELETE FROM `rbac_role_permissions` WHERE `permissionId` IN (1050, 1051);

-- No rbac_linked_permissions rows on purpose: the command ids above are NOT
-- linked to RBAC_PERM_COMMAND_AURORA_VOTE / _SHOP arcs, because those arcs
-- are declared in C++ and resolved at build time. The permission rows only
-- exist so administrators can grant them by name:
--
--   Grant voting to everyone:
--     INSERT INTO rbac_account_permissions (accountId, permissionId, granted)
--     VALUES (<accountId>, 1050, 1);
--
--   Or grant to an entire role:
--     INSERT INTO rbac_role_permissions (roleId, permissionId) VALUES (1, 1051);
--
-- Revoke after an event:
--     DELETE FROM rbac_account_permissions WHERE permissionId = 1050;
