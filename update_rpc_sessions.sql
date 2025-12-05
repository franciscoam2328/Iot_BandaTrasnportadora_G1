-- Actualizar la función RPC para que cuente también en la sesión activa

CREATE OR REPLACE FUNCTION incrementar_contador(p_turno TEXT, p_color TEXT)
RETURNS VOID AS $$
BEGIN
    -- 1. Actualizar contador global del turno (Histórico acumulado)
    IF p_color = 'ROJO' THEN
        UPDATE turnos SET rojo = rojo + 1 WHERE turno = p_turno;
    ELSIF p_color = 'VERDE' THEN
        UPDATE turnos SET verde = verde + 1 WHERE turno = p_turno;
    ELSIF p_color = 'AZUL' THEN
        UPDATE turnos SET azul = azul + 1 WHERE turno = p_turno;
    END IF;

    -- 2. Actualizar contador de la sesión activa (si existe)
    -- Se busca una sesión del turno especificado que no tenga fecha de fin
    IF p_color = 'ROJO' THEN
        UPDATE sesiones SET total_rojo = total_rojo + 1 
        WHERE turno = p_turno AND fin IS NULL;
    ELSIF p_color = 'VERDE' THEN
        UPDATE sesiones SET total_verde = total_verde + 1 
        WHERE turno = p_turno AND fin IS NULL;
    ELSIF p_color = 'AZUL' THEN
        UPDATE sesiones SET total_azul = total_azul + 1 
        WHERE turno = p_turno AND fin IS NULL;
    END IF;
END;
$$ LANGUAGE plpgsql;
