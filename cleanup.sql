-- Script de Limpieza (Reset)
-- Ejecuta esto para borrar las tablas y funciones antiguas antes de crear las nuevas.

-- 1. Borrar tablas (CASCADE elimina dependencias si las hay)
DROP TABLE IF EXISTS lecturas CASCADE;
DROP TABLE IF EXISTS sistema CASCADE;
DROP TABLE IF EXISTS turnos CASCADE;

-- 2. Borrar funciones RPC
DROP FUNCTION IF EXISTS incrementar_contador(text, text);

-- 3. (Opcional) Si tenías políticas RLS antiguas y quieres asegurarte
-- Al borrar las tablas, las políticas asociadas se borran automáticamente.
