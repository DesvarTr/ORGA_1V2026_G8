void leerBluetooth()
{
    if (Serial.available())
    {
        String comando =
            Serial.readStringUntil('\n');

        comando.trim();

        procesarComando(comando);
    }
}
