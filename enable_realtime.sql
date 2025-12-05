-- Habilitar Realtime para las tablas
-- Ejecuta esto en el SQL Editor de Supabase para que la web se actualice sola.

begin;
  -- Eliminar si ya existen (para evitar duplicados)
  drop publication if exists supabase_realtime;

  -- Crear la publicación por defecto (si no existe)
  create publication supabase_realtime;
commit;

-- Agregar las tablas a la publicación de Realtime
alter publication supabase_realtime add table sistema;
alter publication supabase_realtime add table turnos;
alter publication supabase_realtime add table lecturas;
alter publication supabase_realtime add table sesiones;
