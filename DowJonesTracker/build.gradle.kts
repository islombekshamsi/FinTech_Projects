plugins {
    id("application")
    id("org.openjfx.javafxplugin") version "0.0.13"
}

group = "org.example"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

dependencies {
    implementation("com.yahoofinance-api:YahooFinanceAPI:3.17.0")
    testImplementation(platform("org.junit:junit-bom:5.10.0"))
    testImplementation("org.junit.jupiter:junit-jupiter")
}

javafx {
    version = "17"
    modules = listOf("javafx.controls", "javafx.fxml")
}

application {
    mainClass.set("org.example.App")
}

tasks.test {
    useJUnitPlatform()
}