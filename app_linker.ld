SECTIONS
{
  .charger_event_handlers :
  {
    PROVIDE(__start_charger_event_handlers = .);
    KEEP(*(SORT(.charger_event_handlers*)))
    PROVIDE(__stop_charger_event_handlers = .);
  }
} INSERT AFTER .text
