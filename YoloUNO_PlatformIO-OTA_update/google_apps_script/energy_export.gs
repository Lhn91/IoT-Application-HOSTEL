/**
 * Google Apps Script for Energy Consumption Data Export
 * This script receives POST requests from the ESP32 and writes data to Google Sheets
 */

// Configuration
const SPREADSHEET_ID = 'YOUR_SPREADSHEET_ID_HERE'; // Replace this with your Google Sheets ID
const SHEET_NAME = 'Energy_Consumption';

function doPost(e) {
  try {
    // Log incoming request for debugging
    console.log('Received POST request');
    console.log('Content type:', e.postData.type);
    console.log('Raw data:', e.postData.contents);
    
    // Parse JSON data from ESP32
    let data;
    try {
      data = JSON.parse(e.postData.contents);
    } catch (parseError) {
      console.error('JSON parse error:', parseError);
      return ContentService
        .createTextOutput(JSON.stringify({
          'status': 'error',
          'message': 'Invalid JSON format'
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    
    // Validate required fields
    const requiredFields = ['cardId', 'startTime', 'endTime', 'totalEnergy', 'averagePower', 'cost', 'deviceName'];
    for (let field of requiredFields) {
      if (!(field in data)) {
        console.error('Missing field:', field);
        return ContentService
          .createTextOutput(JSON.stringify({
            'status': 'error',
            'message': 'Missing required field: ' + field
          }))
          .setMimeType(ContentService.MimeType.JSON);
      }
    }
    
    // Get the spreadsheet and worksheet
    const spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
    let sheet = spreadsheet.getSheetByName(SHEET_NAME);
    
    // Create sheet if it doesn't exist
    if (!sheet) {
      sheet = spreadsheet.insertSheet(SHEET_NAME);
      
      // Add headers
      const headers = [
        'Timestamp',
        'Card ID', 
        'Start Time',
        'End Time',
        'Duration (seconds)',
        'Energy (kWh)',
        'Average Power (W)',
        'Cost (VND)',
        'Device Name'
      ];
      sheet.getRange(1, 1, 1, headers.length).setValues([headers]);
      
      // Format header row
      const headerRange = sheet.getRange(1, 1, 1, headers.length);
      headerRange.setFontWeight('bold');
      headerRange.setBackground('#4285f4');
      headerRange.setFontColor('white');
    }
    
    // Convert timestamps to readable format
    const currentTime = new Date();
    const startTime = new Date(data.startTime * 1000);
    const endTime = new Date(data.endTime * 1000);
    const duration = data.endTime - data.startTime;
    
    // Prepare row data
    const rowData = [
      currentTime,
      data.cardId,
      startTime,
      endTime,
      duration,
      parseFloat(data.totalEnergy).toFixed(4),
      parseFloat(data.averagePower).toFixed(2),
      parseFloat(data.cost).toFixed(0),
      data.deviceName
    ];
    
    // Add data to sheet
    sheet.appendRow(rowData);
    
    // Get the row number that was just added
    const lastRow = sheet.getLastRow();
    
    // Format the new row
    const dataRange = sheet.getRange(lastRow, 1, 1, rowData.length);
    
    // Format timestamp columns
    sheet.getRange(lastRow, 1).setNumberFormat('yyyy-mm-dd hh:mm:ss'); // Current timestamp
    sheet.getRange(lastRow, 3).setNumberFormat('yyyy-mm-dd hh:mm:ss'); // Start time
    sheet.getRange(lastRow, 4).setNumberFormat('yyyy-mm-dd hh:mm:ss'); // End time
    
    // Format numeric columns
    sheet.getRange(lastRow, 6).setNumberFormat('0.0000'); // Energy (kWh)
    sheet.getRange(lastRow, 7).setNumberFormat('0.00');   // Power (W)
    sheet.getRange(lastRow, 8).setNumberFormat('#,##0');  // Cost (VND)
    
    // Auto-resize columns
    sheet.autoResizeColumns(1, rowData.length);
    
    console.log('Data saved successfully to row:', lastRow);
    
    // Return success response
    return ContentService
      .createTextOutput(JSON.stringify({
        'status': 'success',
        'message': 'Data saved successfully',
        'row': lastRow,
        'timestamp': currentTime.toISOString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
      
  } catch (error) {
    console.error('Error processing request:', error);
    
    // Return error response
    return ContentService
      .createTextOutput(JSON.stringify({
        'status': 'error',
        'message': 'Server error: ' + error.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function doGet(e) {
  // Handle GET requests (for testing)
  return ContentService
    .createTextOutput(JSON.stringify({
      status: 'success',
      message: 'Energy Export API is running',
      timestamp: new Date().toISOString()
    }))
    .setMimeType(ContentService.MimeType.JSON);
}

/**
 * Function to get energy consumption statistics
 * Can be called from ThingsBoard or other services
 */
function getEnergyStats() {
  try {
    const spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
    const sheet = spreadsheet.getSheetByName(SHEET_NAME);
    
    if (!sheet) {
      return {
        status: 'error',
        message: 'No data sheet found'
      };
    }
    
    const dataRange = sheet.getDataRange();
    const values = dataRange.getValues();
    
    if (values.length <= 1) {
      return {
        status: 'success',
        totalSessions: 0,
        totalEnergy: 0,
        totalCost: 0
      };
    }
    
    // Calculate statistics (skip header row)
    let totalEnergy = 0;
    let totalCost = 0;
    const totalSessions = values.length - 1;
    
    for (let i = 1; i < values.length; i++) {
      totalEnergy += parseFloat(values[i][5]) || 0; // Total Energy column
      totalCost += parseFloat(values[i][7]) || 0;   // Cost column
    }
    
    return {
      status: 'success',
      totalSessions: totalSessions,
      totalEnergy: totalEnergy.toFixed(6),
      totalCost: totalCost.toFixed(0),
      averageEnergyPerSession: (totalEnergy / totalSessions).toFixed(6),
      averageCostPerSession: (totalCost / totalSessions).toFixed(0)
    };
    
  } catch (error) {
    return {
      status: 'error',
      message: error.toString()
    };
  }
}

/**
 * Function to export data for a specific card ID
 */
function getCardData(cardId) {
  try {
    const spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
    const sheet = spreadsheet.getSheetByName(SHEET_NAME);
    
    if (!sheet) {
      return {
        status: 'error',
        message: 'No data sheet found'
      };
    }
    
    const dataRange = sheet.getDataRange();
    const values = dataRange.getValues();
    
    // Filter data for specific card ID
    const cardData = [];
    for (let i = 1; i < values.length; i++) {
      if (values[i][1] === cardId) { // Card ID column
        cardData.push({
          timestamp: values[i][0],
          startTime: values[i][2],
          endTime: values[i][3],
          duration: values[i][4],
          totalEnergy: values[i][5],
          averagePower: values[i][6],
          cost: values[i][7],
          deviceName: values[i][8]
        });
      }
    }
    
    return {
      status: 'success',
      cardId: cardId,
      sessions: cardData,
      totalSessions: cardData.length
    };
    
  } catch (error) {
    return {
      status: 'error',
      message: error.toString()
    };
  }
}

// Test function for debugging
function testDoPost() {
  const testData = {
    postData: {
      type: 'application/json',
      contents: JSON.stringify({
        cardId: 'TEST123',
        startTime: Math.floor(Date.now() / 1000) - 1800, // 30 minutes ago
        endTime: Math.floor(Date.now() / 1000),
        totalEnergy: 0.125,
        averagePower: 150.5,
        cost: 437.5,
        deviceName: 'Test Device'
      })
    }
  };
  
  const result = doPost(testData);
  console.log('Test result:', result.getContent());
} 