#ifndef LANGUAGES
#define LANGUAGES


// Uncomment the language you want to display on the screen
#define LANGUAGE_FR
//#define LANGUAGE_EN

class Language
{
  public:
    #ifdef LANGUAGE_FR
    // French weekday and month names for date formatting on display
    const char *days[7] = { "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi" };
    const char *months[12] = { "Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre" };
    const char *forecast_text = "Prevision";
    const char *temperatures_text = "Temperatures";
    const char *battery_text = "Batterie";
    const char *indoor_text = "Salon";
    const char *outdoor_text = "Exterieur";
    const char *other_text = "Bureau";
    #else
    // English weekday and month names for date formatting on display
    const char *days[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    const char *months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    const char *forecast_text = "Forecast";
    const char *temperatures_text = "Temperatures";
    const char *battery_text = "Battery";
    const char *indoor_text = "Indoor";
    const char *outdoor_text = "Outdoor";
    const char *other_text = "Other";
    #endif
};
#endif